#pragma once

enum class Response
{
    OK, // Стандартный Response без дополнительных действий
    BACK, // Ход назад
    REPLAY, // Переиграть игру
    QUIT, // Выход
    CELL // Клетка, в которую мы ходим на данном ходу
};
