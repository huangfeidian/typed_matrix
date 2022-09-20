#include "typed_matrix.h"
#include <iostream>
#include <sstream>

namespace spiritsaway::typed_matrix
{
	typed_matrix* typed_matrix::construct(const std::vector<input_header>& headers, const std::vector<json>& shared_json_table, const std::vector<std::vector<std::uint32_t>>& row_value_indexes)
	{
		if (headers.size() >= std::numeric_limits<std::uint16_t>::max())
		{
			std::cout << "column sz exceed uint16_t max with value " << headers.size() << std::endl;
			return nullptr;
		}

		if (row_value_indexes.size() >= std::numeric_limits<std::uint16_t>::max())
		{
			std::cout << "row sz exceed uint16_t max with value " << row_value_indexes.size() << std::endl;
			return nullptr;
		}
		if (shared_json_table.size() >= std::numeric_limits<std::uint32_t>::max())
		{
			std::cout << "shared_json_table sz exceed uint32_t max with value " << shared_json_table.size() << std::endl;
			return nullptr;
		}
		if (headers.empty())
		{
			std::cout << " header is empty" << std::endl;
			return nullptr;
		}
		std::uint16_t row_sz = row_value_indexes.size();
		std::uint16_t column_sz = headers.size();


		std::vector<column_header> typed_headers(headers.size());
		for (int i = 0; i < headers.size(); i++)
		{
			typed_headers[i].comment = headers[i].comment;
			typed_headers[i].name = headers[i].name;
			typed_headers[i].type_str = headers[i].type_str;
			auto cur_type_desc = container::typed_string_desc::get_type_from_str( headers[i].type_str);
			if (!cur_type_desc)
			{
				std::cout << "cant parse column " << i << " with str " << headers[i].type_str << std::endl;
				return nullptr;
			}
			typed_headers[i].type_desc = cur_type_desc;
		}
		
		if (!typed_headers[0].type_desc)
		{
			std::cout << "head desc empty" << std::endl;
			return nullptr;
		}
		auto cur_row_key_type = typed_headers[0].type_desc->m_type;
		if (cur_row_key_type != container::basic_value_type::number_uint && cur_row_key_type != container::basic_value_type::str)
		{
			std::cout << "invalid row key type " << typed_headers[0].type_desc->encode() << std::endl;
			return nullptr;
		}



		std::unordered_map<std::string, std::uint16_t> str_row_indexes;
		std::unordered_map<std::uint32_t, std::uint16_t> int_row_indexes;
		for (std::uint16_t i = 0; i < row_value_indexes.size(); i++)
		{

			if (cur_row_key_type == container::basic_value_type::number_uint)
			{
				auto cur_row_key = shared_json_table[row_value_indexes[i][0]].get<std::uint32_t>();
				auto cur_key_iter = int_row_indexes.find(cur_row_key);
				if (cur_key_iter == int_row_indexes.end())
				{
					int_row_indexes[cur_row_key] = i;
				}
				else
				{
					std::cout << "duplicated key " << cur_row_key << std::endl;;
					return nullptr;
				}

			}
			else
			{
				auto cur_row_key = shared_json_table[row_value_indexes[i][0]];
				auto cur_key_iter = str_row_indexes.find(cur_row_key);
				if (cur_key_iter == str_row_indexes.end())
				{
					str_row_indexes[cur_row_key] = i;
				}
				else
				{
					std::cout << "duplicated key " << cur_row_key<<std::endl;
					return nullptr;
				}

			}
		}
		if(cur_row_key_type == container::basic_value_type::number_uint)
		{
			auto cur_matrix = new typed_matrix(typed_headers, shared_json_table, row_value_indexes, int_row_indexes);
			return cur_matrix;
		}
		else
		{
			auto cur_matrix = new typed_matrix(typed_headers, shared_json_table, row_value_indexes, str_row_indexes);
			return cur_matrix;
		}

	}
	typed_matrix::typed_matrix( const std::vector<column_header>& columns, const std::vector<json>& shared_json_table, const std::vector<std::vector<std::uint32_t>>& cell_value_indexes, const std::unordered_map<std::string, std::uint16_t>& row_indexes)
		: m_row_sz(row_indexes.size())
		, m_column_sz(columns.size())
		, m_columns(columns)
		, m_is_str_key(true)
		, m_column_indexes(init_column_indexes(columns))
		, m_cell_json_values(shared_json_table)
		, m_cell_value_indexes(cell_value_indexes)
		, m_str_row_indexes(row_indexes)
	{

	}

	typed_matrix::typed_matrix(const std::vector<column_header>& columns, const std::vector<json>& shared_json_table, const std::vector<std::vector<std::uint32_t>>& cell_value_indexes, const std::unordered_map<std::uint32_t, std::uint16_t>& row_indexes)
		: m_row_sz(row_indexes.size())
		, m_column_sz(columns.size())
		, m_columns(columns)
		, m_is_str_key(false)
		, m_column_indexes(init_column_indexes(columns))
		, m_cell_json_values(shared_json_table)
		, m_cell_value_indexes(cell_value_indexes)
		, m_int_row_indexes(row_indexes)
	{

	}
	typed_row typed_matrix::get_row(const std::string& cur_row_key) const
	{
		typed_row result;
		if (!m_is_str_key)
		{
			return result;
		}
		auto cur_iter = m_str_row_indexes.find(cur_row_key);
		if (cur_iter == m_str_row_indexes.end())
		{
			return result;
		}
		result = typed_row(this, cur_iter->second + 1);
		return result;
	}
	typed_row typed_matrix::get_row(const std::uint32_t& cur_row_key)const
	{
		typed_row result;
		if (m_is_str_key)
		{
			return result;
		}
		auto cur_iter = m_int_row_indexes.find(cur_row_key);
		if (cur_iter == m_int_row_indexes.end())
		{
			return result;
		}
		result = typed_row(this, cur_iter->second + 1);
		return result;
	}
	typed_matrix::column_index typed_matrix::get_column_idx(const std::string& cur_column_key) const
	{
		typed_matrix::column_index result;
		auto cur_iter = m_column_indexes.find(cur_column_key);
		if (cur_iter == m_column_indexes.end())
		{
			return result;
		}
		result.set_value(cur_iter->second + 1);
		return result;
	}
	const json& typed_matrix::get_cell_safe(const std::uint16_t& row_idx, const std::uint16_t col_idx)const
	{
		return m_cell_json_values[m_cell_value_indexes[row_idx][col_idx]];
	}
	const json& typed_matrix::get_cell(const typed_row& row_idx, const typed_matrix::column_index col_idx) const
	{
		if (row_idx.m_matrix != this)
		{
			return m_cell_json_values[0];
		}
		if (row_idx.m_row_index == 0)
		{
			return m_cell_json_values[0];
		}
		if (!col_idx.valid())
		{
			return m_cell_json_values[0];
		}
		std::uint16_t cur_row_idx = row_idx.m_row_index - 1;
		std::uint16_t cur_column_idx = col_idx.value() - 1;
		if (cur_row_idx >= m_row_sz)
		{
			return m_cell_json_values[0];
		}
		if (cur_column_idx >= m_column_sz)
		{
			return m_cell_json_values[0];
		}
		return get_cell_safe(cur_row_idx, cur_column_idx);
	}

	const json& typed_matrix::get_cell(const typed_row& row_idx, const std::string& cur_column_key) const
	{
		if (row_idx.m_matrix != this)
		{
			return m_cell_json_values[0];
		}
		if (row_idx.m_row_index == 0)
		{
			return m_cell_json_values[0];
		}
		auto cur_iter = m_column_indexes.find(cur_column_key);
		if (cur_iter == m_column_indexes.end())
		{
			return m_cell_json_values[0];
		}
		std::uint16_t cur_row_idx = row_idx.m_row_index - 1;
		std::uint16_t cur_column_idx = cur_iter->second;
		if (cur_row_idx >= m_row_sz)
		{
			return m_cell_json_values[0];
		}

		return get_cell_safe(cur_row_idx, cur_column_idx);
	}


	typed_matrix::~typed_matrix()
	{

	}
	json typed_matrix::to_json() const
	{
		json result;
		json::array_t headers(m_columns.size());
		for (int i = 0; i < m_columns.size(); i++)
		{
			json::array_t cur_column_header(3);
			cur_column_header[0] = m_columns[i].name;
			cur_column_header[1] = m_columns[i].comment;
			cur_column_header[2] = m_columns[i].type_str;
			headers[i] = cur_column_header;
		}
		result["headers"] = headers;
		result["shared_json_table"] = m_cell_json_values;
		result["row_matrix"] = m_cell_value_indexes;
		result["extras"] = json::array_t{};
		return result;
		

	}

	std::string typed_matrix::to_comment_json() const
	{
		std::ostringstream cur_oss;
		cur_oss << "{\n";
		cur_oss << "\t\"headers\": ";
		json::array_t headers(m_columns.size());
		for (int i = 0; i < m_columns.size(); i++)
		{
			json::array_t cur_column_header(3);
			cur_column_header[0] = m_columns[i].name;
			cur_column_header[1] = m_columns[i].comment;
			cur_column_header[2] = m_columns[i].type_str;
			headers[i] = cur_column_header;
		}
		cur_oss << json(headers).dump(1, '\t') << ",\n";

		cur_oss << "\t\"shared_json_table\": "<<json(m_cell_json_values).dump(1, '\t') << "," << std::endl;
		cur_oss << "\t\"extras\" : [],\n";
		cur_oss << "\t\"row_matrix\": [\n";
		for (int i = 0; i < m_cell_value_indexes.size(); i++)
		{
			cur_oss << "\t\t[\n";
			for (int j = 0; j < m_cell_value_indexes[i].size(); j++)
			{
				if (j + 1 == m_cell_value_indexes[i].size())
				{
					cur_oss << "\t\t\t" << m_cell_value_indexes[i][j] << " // (" << m_columns[j].name << ", " << m_cell_json_values[m_cell_value_indexes[i][j]].dump() <<")\n";
				}
				else
				{
					cur_oss << "\t\t\t" << m_cell_value_indexes[i][j] << ", // (" << m_columns[j].name << ", " << m_cell_json_values[m_cell_value_indexes[i][j]].dump() << ")\n";
				}
				
				
			}
			if (i + 1== m_cell_value_indexes.size())
			{
				cur_oss << "\t\t]\n";
			}
			else
			{
				cur_oss << "\t\t],\n";
			}
		}
		cur_oss << "\t]\n}";
		return cur_oss.str();
	}

	typed_matrix* typed_matrix::from_json(const json& json_matrix)
	{
		std::vector<std::array<std::string, 3>> headers;
		std::vector<json> shared_json_table;
		std::vector<std::vector<std::uint32_t>> cells;
		std::vector<std::array<json, 3>> extras;
		try
		{
			json_matrix.at("headers").get_to(headers);
			json_matrix.at("shared_json_table").get_to(shared_json_table);
			json_matrix.at("row_matrix").get_to(cells);
			json_matrix.at("extras").get_to(extras);
		}
		catch (std::exception& e)
		{
			std::cout << "typed_matrix::from_json failed with err " <<e.what()<< std::endl;
			return nullptr;
		}
		std::vector<input_header> cur_input_header(headers.size());
		std::unordered_map<std::string, std::uint32_t> header_to_idx;
		for (int i = 0; i < headers.size(); i++)
		{
			cur_input_header[i].name = headers[i][0];
			cur_input_header[i].comment = headers[i][1];
			cur_input_header[i].type_str = headers[i][2];
			header_to_idx[headers[i][0]] = i;
		}
		std::unordered_map<std::string, std::uint32_t> str_key_indexes;
		std::unordered_map<std::uint32_t, std::uint32_t> int_key_indexes;
		for (int i = 0; i < cells.size(); i++)
		{
			if (cells[i].size() != headers.size())
			{
				std::cout << "column sz for row " << i << " not match" << std::endl;
				return nullptr;
			}
			for (int j = 0; j < headers.size(); j++)
			{
				if (cells[i][j] >= shared_json_table.size())
				{
					std::cout << "value idx for cell(" << i << "," << j << ") exceed max value size " << std::endl;
					return nullptr;
				}
			}
			const auto& cur_row_key = shared_json_table[cells[i][0]];
			if (cur_row_key.is_string())
			{
				str_key_indexes[cur_row_key.get<std::string>()] = i;
			}
			else
			{
				int_key_indexes[cur_row_key.get<std::uint32_t>()] = i;
			}
		}
		for (const auto& one_extra : extras)
		{
			if (!one_extra[1].is_string())
			{
				continue;
			}
			auto cur_column_iter = header_to_idx.find(one_extra[1].get<std::string>());

			if (one_extra[0].is_string())
			{
				auto cur_row_iter = str_key_indexes.find(one_extra[0].get<std::string>());
				if (cur_row_iter == str_key_indexes.end())
				{
					continue;
				}
				if (one_extra[2].is_null())
				{
					cells[cur_row_iter->second][cur_column_iter->second] = 0;
				}
				else
				{
					shared_json_table.push_back(one_extra[2]);
					cells[cur_row_iter->second][cur_column_iter->second] = shared_json_table.size() - 1;
				}
				
				
			}
			else if (one_extra[0].is_number_unsigned())
			{
				auto cur_row_iter = int_key_indexes.find(one_extra[0].get<std::uint32_t>());
				if (cur_row_iter == int_key_indexes.end())
				{
					continue;
				}
				if (one_extra[2].is_null())
				{
					cells[cur_row_iter->second][cur_column_iter->second] = 0;
				}
				else
				{
					shared_json_table.push_back(one_extra[2]);
					cells[cur_row_iter->second][cur_column_iter->second] = shared_json_table.size() - 1;
				}
			}
			else
			{
				continue;
			}
		}
		return construct(cur_input_header, shared_json_table, cells);
	}
	std::unordered_map<std::string, std::uint16_t> typed_matrix::init_column_indexes(const std::vector<column_header>& in_columns)
	{
		std::unordered_map<std::string, std::uint16_t> result;
		for (std::uint16_t i = 0; i < in_columns.size(); i++)
		{
			result[in_columns[i].name] = i;
		}
		return result;
	}
	typed_row::typed_row()
		: m_matrix(nullptr)
		, m_row_index(0)
	{

	}
	typed_matrix::column_index typed_row::get_column_idx(const std::string& cur_column_key) const
	{
		if (!m_matrix || !m_row_index)
		{
			return {};
		}
		else
		{
			return m_matrix->get_column_idx(cur_column_key);
		}
	}
	const json& typed_row::get_cell(typed_matrix::column_index column_idx) const
	{
		static const json invalid_result;
		if (!m_matrix)
		{
			return invalid_result;
		}
		return m_matrix->get_cell(*this, column_idx);
	}
	const json& typed_row::get_cell(const std::string& cur_column_key) const
	{
		static const json invalid_result;
		if (!m_matrix)
		{
			return invalid_result;
		}
		return m_matrix->get_cell(*this, cur_column_key);
	}
	bool typed_row::valid() const
	{
		return m_matrix && m_row_index;
	}
	typed_row::typed_row(const typed_matrix* matrix, std::uint16_t row_index)
		: m_matrix(matrix)
		, m_row_index(row_index)
	{

	}
	typed_row typed_matrix::begin_row() const
	{
		return typed_row(this, 1);
	}

	typed_row typed_matrix::next_row(const typed_row& pre_row) const
	{
		if (!pre_row.valid())
		{
			return {};
		}
		if (pre_row.m_matrix != this)
		{
			return {};
		}
		if (pre_row.m_row_index  == m_row_sz)
		{
			return {};
		}
		return typed_row(this, pre_row.m_row_index + 1);
	}
	typed_matrix::column_index typed_matrix::begin_column() const
	{
		column_index result;
		if (!m_columns.empty())
		{
			result.set_value(1);
		}
		return result;
	}
	typed_matrix::column_index typed_matrix::next_column(typed_matrix::column_index pre_column) const
	{
		if (!pre_column.valid())
		{
			return pre_column;
		}
		column_index result;
		if (pre_column.value() < m_columns.size())
		{
			result.set_value(pre_column.value() + 1);
		}
		return result;
	}
	const column_header* typed_matrix::get_column_header(column_index col_idx) const
	{
		if (!col_idx.valid())
		{
			return nullptr;
		}
		if (col_idx.value() > m_columns.size())
		{
			return nullptr;
		}
		return &m_columns[col_idx.value() - 1];
	}

}
