#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
  public:
    Config()
    {
        reload();
    }

    void reload() // Функция загружает настройки из файла "settings.json" в переменную config
    {
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }

    auto operator()(const string &setting_dir, const string &setting_name) const  // Оператор получает получать из настроек нужную нам настройку,
    {                                                                             // обратившись к разделу и подразделу настроек. Например "Bot", "NoRandom"
        return config[setting_dir][setting_name];
    }

  private:
    json config;
};
