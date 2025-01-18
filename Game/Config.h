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

    void reload() // ������� ��������� ��������� �� ����� "settings.json" � ���������� config
    {
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }

    auto operator()(const string &setting_dir, const string &setting_name) const  // �������� �������� �������� �� �������� ������ ��� ���������,
    {                                                                             // ����������� � ������� � ���������� ��������. �������� "Bot", "NoRandom"
        return config[setting_dir][setting_name];
    }

  private:
    json config;
};
