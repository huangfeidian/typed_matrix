#pragma once
#include "typed_matrix.h"

namespace spiritsaway::typed_matrix
{
	class matrix_exporter
	{
	public:
		// return workbook last write time

		std::string export_workbook(const std::string& xlsx_workbook_path, const std::unordered_map<std::string, std::string>& sheet_map, const std::string& dest_folder);
	};
}

