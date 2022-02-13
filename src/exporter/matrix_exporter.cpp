#include "matrix_exporter.h"
#include <xlsx_reader/xlsx_workbook.h>
#include <xlsx_reader/xlsx_worksheet.h>
#include <fstream>
#include <filesystem>
#include <chrono>
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
	std::string matrix_exporter::export_workbook(const std::string& xlsx_workbook_path, const std::unordered_map<std::string, std::string>& sheet_map, const std::string& dest_folder)
	{
		auto archive_content = std::make_shared<spiritsaway::xlsx_reader::archive>(xlsx_workbook_path);
		xlsx_reader::workbook<xlsx_reader::worksheet> current_workbook(archive_content);
		const int value_begin_row = 4; //行号从1开始 前面三行分别是name type comment
		for (const auto& one_sheet : current_workbook._worksheets)
		{
			auto cur_sheet_name = one_sheet->get_name();
			auto sheet_iter = sheet_map.find(std::string(cur_sheet_name));
			if (sheet_iter == sheet_map.end())
			{
				continue;
			}
			if (one_sheet->get_max_row() < 3)
			{
				std::cout << " sheet " << cur_sheet_name << " row size " << one_sheet->get_max_row() << " invalid" << std::endl;
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
					std::cout << "sheet " << cur_sheet_name << " column " << i << " has name or type empty " << cur_header_name << ": " << cur_header_type << std::endl;
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
			std::unordered_map<std::string, std::uint32_t> cur_sheet_shared_string_table;
			cur_sheet_shared_string_table[std::string()] = 0;
			std::vector<std::vector<std::uint32_t>> cur_sheet_cells(one_sheet->get_max_row() - value_begin_row + 1, std::vector<std::uint32_t>(header_name_row.size() - 1));
			for (int i = 4; i < one_sheet->get_max_row() + 1; i++)
			{
				const auto& cur_row_info = one_sheet->get_row(i);
				for (int j = 1; j < header_name_row.size(); j++)
				{
					auto cur_cell_str = std::string(current_workbook.get_shared_string(cur_row_info[j]));
					auto cur_cell_str_iter = cur_sheet_shared_string_table.find(cur_cell_str);
					std::uint32_t new_shared_str_idx = 0;
					if (cur_cell_str_iter == cur_sheet_shared_string_table.end())
					{
						new_shared_str_idx = cur_sheet_shared_string_table.size();
						cur_sheet_shared_string_table[cur_cell_str] = new_shared_str_idx;
					}
					else
					{
						new_shared_str_idx = cur_cell_str_iter->second;
					}
					cur_sheet_cells[i - 4][j - 1] = new_shared_str_idx;
				}

			}
			std::vector<std::string> sst_vec(cur_sheet_shared_string_table.size());
			for (const auto& one_pair : cur_sheet_shared_string_table)
			{
				sst_vec[one_pair.second] = one_pair.first;
			}
			auto cur_typed_matrix = typed_matrix::construct(cur_sheet_headers, sst_vec, cur_sheet_cells);
			if (!cur_typed_matrix)
			{
				std::cout << "fail to construct matrix for sheet " << cur_sheet_name << std::endl;
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
		auto cur_workbook_path = std::filesystem::path(xlsx_workbook_path);
		auto ftime = std::filesystem::last_write_time(cur_workbook_path);
		//auto cur_f_duration = std::chrono::duration_cast<std::chrono::milliseconds>(ftime.time_since_epoch());
		//auto cur_time_t = std::chrono::system_clock::to_time_t(std::chrono::time_point<std::chrono::system_clock>(cur_f_duration));
		auto cur_time_t = to_time_t(ftime);
		struct tm* timeinfo;
		char buffer[80];

		timeinfo = localtime(&cur_time_t);

		strftime(buffer, sizeof(buffer), "%Y_%m_%d_%H_%M_%S", timeinfo);
		return std::string(buffer);

	}

}