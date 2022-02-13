#include "matrix_exporter.h"
#include <iostream>
int main()
{
	std::string xlsx_path = "../../../data/xlsx/sample1.xlsx";
	std::unordered_map<std::string, std::string> sheet_map = {
		{"value_test", "value_test.json"},
		{"ref_test", "ref_test.json"},
		{"all_colors", "all_colors.json"}
	};
	std::string json_path = "../../../data/export/";
	auto cur_exporter = spiritsaway::typed_matrix::matrix_exporter();
	auto cur_ts = cur_exporter.export_workbook(xlsx_path, sheet_map, json_path);
	std::cout << "cur file stamp is " << cur_ts << std::endl;
}