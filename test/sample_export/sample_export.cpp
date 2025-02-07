#include "matrix_exporter.h"
#include <iostream>
#include <sstream>
#include <fstream>

json load_json_file(const std::string& filepath)
{
	std::ifstream t(filepath);
	std::stringstream buffer;
	buffer << t.rdbuf();
	auto cur_file_content = buffer.str();
	// std::cout<<"cur file_content is "<<cur_file_content<<std::endl;
	if (!json::accept(cur_file_content, true))
	{
		return {};
	}
	return json::parse(cur_file_content, nullptr, true, true);
}
int main()
{
	//std::string xlsx_path = u8"../../../data/xlsx/space.xlsx";
	//std::unordered_map<std::string, std::string> sheet_map = {
	//	{u8"宝石道具表", "space.json"},
	//};
	std::string json_path = "../../../data/export/";
	//auto cur_exporter = spiritsaway::typed_matrix::matrix_exporter();
	//cur_exporter.export_workbook(xlsx_path, sheet_map, json_path, true);
	std::string temp_path = json_path + "space.json";
	auto cur_json = load_json_file(temp_path);
	auto cur_matrix = spiritsaway::typed_matrix::typed_matrix::from_json(cur_json);
	auto cur_space_sysd_1 = cur_matrix->get_row(1);
	auto cur_space_sysd_2 = cur_space_sysd_1;
	std::string cur_map_path;
	cur_space_sysd_2.expect_value("navi_map", cur_map_path);
	return cur_matrix ? 1 : 0;
}