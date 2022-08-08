#include "matrix_exporter.h"
#include <xlsx_reader/xlsx_workbook.h>
#include <xlsx_reader/xlsx_typed_worksheet.h>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <sstream>
namespace
{
	template <typename TP>
	std::time_t to_time_t(TP tp)
	{
		using namespace std::chrono;
		auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
			+ system_clock::now());
		return system_clock::to_time_t(sctp);
	}
	std::string load_file_content(const std::string& file_name)
	{
		std::ifstream t(file_name);
		if (!t)
		{
			return {};
		}
		std::stringstream buffer;
		buffer << t.rdbuf();
		return buffer.str();
	}
}

namespace spiritsaway::typed_matrix
{
	void matrix_exporter::export_workbook(const std::string& xlsx_workbook_path, const std::unordered_map<std::string, std::string>& sheet_map, const std::string& dest_folder)
	{
		std::cout << "begin export workbook " << xlsx_workbook_path << std::endl;
		auto archive_content = std::make_shared<spiritsaway::xlsx_reader::archive>(xlsx_workbook_path);
		xlsx_reader::workbook<xlsx_reader::typed_worksheet> current_workbook(archive_content);
		
		for (const auto& one_sheet : current_workbook.m_worksheets)
		{
			const int value_begin_row = one_sheet->value_begin_row(); //行号从1开始 前面三行分别是name type comment
			auto cur_sheet_name = one_sheet->get_name();
			std::string debug_sheet_name = xlsx_workbook_path + ":" + std::string(cur_sheet_name);
			auto sheet_iter = sheet_map.find(std::string(cur_sheet_name));
			if (sheet_iter == sheet_map.end())
			{
				continue;
			}
			if (one_sheet->get_max_row() < 3)
			{
				std::cout << " sheet " << debug_sheet_name << " row size " << one_sheet->get_max_row() << " invalid" << std::endl;
				continue;
			}
			const auto& header_name_row = one_sheet->get_row(1);
			// 列号也是从1开始的
			std::vector<input_header> cur_sheet_headers(header_name_row.size() - 1);
			
			for (std::size_t i = 1; i < header_name_row.size(); i++)
			{
				auto cur_header_name = one_sheet->get_cell(1, i);
				auto cur_header_type = one_sheet->get_cell(2, i);
				auto cur_header_comment = one_sheet->get_cell(3, i);
				if (cur_header_name.empty() || cur_header_type.empty())
				{
					std::cout << "sheet " << debug_sheet_name << " column " << i << " has name or type empty " << cur_header_name << ": " << cur_header_type << std::endl;
					break;
					cur_sheet_headers.clear();
				}
				cur_sheet_headers[i - 1].comment = cur_header_comment;
				cur_sheet_headers[i - 1].name = cur_header_name;
				cur_sheet_headers[i - 1].type_str = cur_header_type;
			}
			if (cur_sheet_headers.empty())
			{
				continue;
			}
			
			
			std::vector<std::vector<std::uint32_t>> temp_cell_value_matrix(one_sheet->get_max_row() - one_sheet->value_begin_row() + 1, std::vector<std::uint32_t>(cur_sheet_headers.size(), 0));
			for (int i = one_sheet->value_begin_row(); i <= one_sheet->max_rows; i++)
			{
				for (int j = 1; j < one_sheet->max_columns; j++)
				{
					temp_cell_value_matrix[i - one_sheet->value_begin_row()][j - 1] = one_sheet->cell_value_indexes()[one_sheet->get_cell_value_index_pos(i, j)];
				}
			}
			auto cur_typed_matrix = typed_matrix::construct(cur_sheet_headers, one_sheet->cell_json_values(), temp_cell_value_matrix);
			if (!cur_typed_matrix)
			{
				std::cout << "fail to construct matrix for sheet " << debug_sheet_name << std::endl;
				continue;
			}

			auto cur_matrix_json = cur_typed_matrix->to_json();
			auto dest_file_path = dest_folder + sheet_iter->second;
			auto cur_json_str = cur_matrix_json.dump(4);
			auto pre_file_str = load_file_content(dest_file_path);
			if (cur_json_str != pre_file_str)
			{
				std::ofstream dest_file_os(dest_file_path);
				dest_file_os << cur_json_str;
			}
			
			delete cur_typed_matrix;

		}
		

	}

}