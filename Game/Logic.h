#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
  public:
    Logic(Board *board, Config *config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine (
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

    vector<move_pos> find_best_turns(const bool color) {
        // Очищаем вектор ходов и вектор наилучших состояний
        next_move.clear();
        next_best_state.clear();
        find_first_best_turn(board->get_board(), color, -1, -1, 0);
        vector<move_pos> res;
        int state = 0;
        do {
            res.push_back(next_move[state]);
            state = next_best_state[state]; // переходим в следующее состояние
        } while (state != -1 && next_move[state].x != -1); // цикл завершаем если следующий ход или состояние == -1
        return res;
    }

private:
    // Первая последовательность ходов первого игрока
    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
        double alpha = -1) {
        next_move.emplace_back(-1, -1, -1, -1); // Изначально ход пустой
        next_best_state.push_back(-1); // состояние тоже
        if (state != 0) { // если состояние не нулевое - считаем все возможные ходы для нашего цвета (для 0 посчитано ранее в game.h)
            find_turns(x, y, mtx);
        }
        // создаем копии состояний чтобы дальнейшие рекурсии не затирали текущие
        auto now_turns = turns;
        auto now_have_beats = have_beats;

        if (!now_have_beats && state != 0) { // нечего бить и не нулевое состояние
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);
        }
        // перебираем все ходы

        double best_score = -1;
        for (auto turn : now_turns) {
            size_t new_state = next_move.size();
            double score;
            if (now_have_beats) { // если бьем
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, new_state, best_score);
            }
            else { // если никого не бьем переходим в рекурсию
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
            }
            if (score > best_score) { // мы нашли новое лучшее состояние, записываем его в лучшее состояние из текущего
                best_score = score;
                next_move[state] = turn;
                next_best_state[state] = (now_have_beats ? new_state : -1);
            }
        }
        return best_score;

    }

    // Просчет относительно состояния, просчитанного в функции find_first_best_turn
    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
        double beta = INF + 1, const POS_T x = -1, const POS_T y = -1) {
        if (depth == Max_depth) {
            return calc_score(mtx, (depth % 2 == color));
        }
        if (x != -1) { // если у нас есть серия побитий строим ходы от фигуры
            find_turns(x, y, mtx);
        }
        else { // иначе мы не знаем от какой фигуры строить, ищем ходы только от цвета
            find_turns(color, mtx);
        }
        // создаем копии состояний чтобы дальнейшие рекурсии не затирали текущие
        auto now_turns = turns;
        auto now_have_beats = have_beats;

        if (!now_have_beats && x != 1) { // нечего бить и не нулевое состояние
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        if (turns.empty()) { // если ходов нет, то игрок проиграл
            return (depth % 2 ? 0 : INF); 
        }

        double min_score = INF + 1;
        double max_score = -1;

        for (auto turn : now_turns) {
            double score;
            if (now_have_beats) {
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2); // если бьем - подсчитываем дальше цепочку
            }
            else {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta); // если нет, ход переходит другому игроку
            }
            min_score = min(min_score, score);
            max_score = max(max_score, score);

            // Оптимизация по alpha-beta отсечению
            if (depth % 2) {
                alpha = max(alpha, max_score);
            }
            else {
                beta = min(beta, min_score);
            }
            if (optimization != "O0" && alpha > beta) {
                break;
            }
            if (optimization != "O2" && alpha == beta) {
                return (depth % 2 ? max_score + 1 : min_score - 1);
            }
        }
        return (depth % 2 ? max_score : min_score);

    }


    // Функция производит ход фигуры turn на матрице mtx
    // В зависимости от хода пересчитывает матрицу
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0;
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2;
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y];
        mtx[turn.x][turn.y] = 0;
        return mtx;  // и возвращает копию матрицы
    }

    // функция подсчета оценки состояний возможных исходов.
    double calc_score(const vector<vector<POS_T>> &mtx, const bool first_bot_color) const
    {
        double w = 0, wq = 0, b = 0, bq = 0;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1); // белых пешек
                wq += (mtx[i][j] == 3); // белых королев
                b += (mtx[i][j] == 2); // черных пешек
                bq += (mtx[i][j] == 4); // черных королев
                if (scoring_mode == "NumberAndPotential") // тип подсчета - количество и потенциал, в зависимости от положения пешек на доске + q_coef королевы
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i);
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }
        if (w + wq == 0)
            return INF; // максимально возможное состояние
        if (b + bq == 0)
            return 0; // минимальное возможное состояние
        int q_coef = 4;
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5;
        }
        return (b + bq * q_coef) / (w + wq * q_coef);
    }


    // Публичные функции find_turns которые ссылаются на приватные, в зависимости от принятых параметров
public:
    void find_turns(const bool color) 
    {
        find_turns(color, board->get_board()); // и вызывает приватную find_turns с этими параметрами
    }

    void find_turns(const POS_T x, const POS_T y) 
    {
        find_turns(x, y, board->get_board()); // и вызывает приватную find_turns с этими параметрами
    }

    // Ищет все возможные ходы, если передается цвет
private: 
    void find_turns(const bool color, const vector<vector<POS_T>> &mtx) // принимает цвет и матрицу состояния игры
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j) // Проходим по всем клеткам
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    find_turns(i, j, mtx); // Если клетка совпадает с цветом, запускаем еще одну функцию find_turns, только уже по кооржинатам (описана ниже)
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        shuffle(turns.begin(), turns.end(), rand_eng); // случайно выбираем ход
        have_beats = have_beats_before;
    }

    // Ищет все возможные ходы, если передаются координаты типа POS_T
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>> &mtx) // принимает цвет и матрицу состояния игры
    {
        turns.clear();
        have_beats = false;
        POS_T type = mtx[x][y];
        // Проверяет побития
        switch (type) //тип фигуры
        {
        case 1: // пешка белая
        case 2: // пешка черная
            // Проверяет проходом все клетки
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;
        default:
            // либо это королева (описывается логика ходов побитий для королевы)
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }
        // Проверка вариантов походить (без побития)
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }
        switch (type)
        {
        case 1:
        case 2:
            // Проверяем клетки
            {
                POS_T i = ((type % 2) ? x - 1 : x + 1);
                for (POS_T j = y - 1; j <= y + 1; j += 2)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                        continue;
                    turns.emplace_back(x, y, i, j);
                }
                break;
            }
        default:
            // Для королевы (логика зодов королевы)
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break;
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }

  public:
    vector<move_pos> turns; // вектор ходов, которые находятся с помощью функции find_turns
    bool have_beats; // является ли ход побитием
    int Max_depth; // максимальная глубина просчета (берется из BotLevel конфигурации

  private:
    default_random_engine rand_eng; // переменная для случайного выбора хода бота в случае равнозначной оценки
    string scoring_mode; // отвечает за функционал подсчета в функции calc_score
    string optimization; // тип оптимизации, O0 - нет оптимизации, О1 - строгое alpha-beta отсечение, О2 - не строгое.
    vector<move_pos> next_move; // вектор ходов для восстановления цепочек ходов про просчете
    vector<int> next_best_state; // вектор следующего лучшего состояния
    Board *board; // уазатель на объект класса доска
    Config *config; // уазатель на объект класса config
};
