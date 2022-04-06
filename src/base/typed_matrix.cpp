#include "typed_matrix.h"
#include <iostream>
#include "typed_string/arena_typed_string_parser.h"

namespace spiritsaway::typed_matrix
{
	typed_matrix* typed_matrix::construct(const std::vector<input_header>& headers, const std::vector<std::string>& shared_string_table, const std::vector<std::vector<std::uint32_t>>& row_values)
	{
		if (headers.size() >= std::numeric_limits<std::uint16_t>::max())
		{
			std::cout << "column sz exceed uint16_t max with value " << headers.size() << std::endl;
			return nullptr;
		}

		if (row_values.size() >= std::numeric_limits<std::uint16_t>::max())
		{
			std::cout << "row sz exceed uint16_t max with value " << row_values.size() << std::endl;
			return nullptr;
		}
		if (shared_string_table.size() >= std::numeric_limits<std::uint32_t>::max())
		{
			std::cout << "shared_string_table sz exceed uint32_t max with value " << shared_string_table.size() << std::endl;
			return nullptr;
		}
		if (headers.empty())
		{
			std::cout << " header is empty" << std::endl;
			return nullptr;
		}
		std::uint16_t row_sz = row_values.size();
		std::uint16_t column_sz = headers.size();
		for (std::uint16_t i = 0; i < row_values.size(); i++)
		{
			if (row_values[i].size() != column_sz)
			{
				std::cout << "row " << i << " has invalid column sz " << row_values[i].size() << std::endl;
				return nullptr;
			}
		}
		memory::arena* header_arena = new memory::arena(1024);

		std::vector<column_header> typed_headers(headers.size());
		for (int i = 0; i < headers.size(); i++)
		{
			typed_headers[i].comment = headers[i].comment;
			typed_headers[i].name = headers[i].name;
			typed_headers[i].type_str = headers[i].type_str;
			auto cur_type_desc = container::typed_string_desc::get_type_from_str(header_arena, headers[i].type_str);
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
		if (cur_row_key_type != container::basic_value_type::number_32 && cur_row_key_type != container::basic_value_type::string)
		{
			std::cout << "invalid row key type " << typed_headers[0].type_desc->to_string() << std::endl;
			return nullptr;
		}


		std::vector<std::uint32_t> cell_strs(std::uint32_t(row_sz) * column_sz);
		std::uint32_t sst_adjust = 0;
		if (shared_string_table.empty() || !shared_string_table[0].empty())
		{
			sst_adjust = 1;
		}
		std::vector<std::string> new_sst(shared_string_table.size() + sst_adjust);
		if (sst_adjust)
		{
			new_sst[0] = std::string();
		}
		std::unordered_map<std::string, std::uint16_t> str_row_indexes;
		std::unordered_map<int, std::uint16_t> int_row_indexes;
		std::copy(shared_string_table.begin(), shared_string_table.end(), new_sst.data() + sst_adjust);
		for (std::uint16_t i = 0; i < row_values.size(); i++)
		{

			if (cur_row_key_type == container::basic_value_type::number_32)
			{
				auto cur_row_key = std::stoi(shared_string_table[row_values[i][0]]);
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
				auto cur_row_key = shared_string_table[row_values[i][0]];
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
			for (std::uint16_t j = 0; j < headers.size(); j++)
			{
				auto cur_sst_idx = row_values[i][j];
				if (shared_string_table[cur_sst_idx].empty())
				{
					cell_strs[i * column_sz + j] = 0;
				}
				else
				{
					cell_strs[i * column_sz + j] = cur_sst_idx + sst_adjust;
				}
			}
		}
		if(cur_row_key_type == container::basic_value_type::number_32)
		{
			auto cur_matrix = new typed_matrix(header_arena, typed_headers, new_sst, cell_strs, int_row_indexes);
			return cur_matrix;
		}
		else
		{
			auto cur_matrix = new typed_matrix(header_arena, typed_headers, new_sst, cell_strs, str_row_indexes);
			return cur_matrix;
		}

	}
	typed_matrix::typed_matrix(memory::arena* header_arena, const std::vector<column_header>& columns, const std::vector<std::string>& shared_string_table, const std::vector<std::uint32_t>& cell_strs, const std::unordered_map<std::string, std::uint16_t>& row_indexes)
		: m_row_sz(row_indexes.size())
		, m_column_sz(columns.size())
		, m_columns(columns)
		, m_is_str_key(true)
		, m_column_indexes(init_column_indexes(columns))
		, m_shared_str_table(shared_string_table)
		, m_cell_strs(cell_strs)
		, m_str_row_indexes(row_indexes)
		, m_cell_values(row_indexes.size())
		, m_header_arena(header_arena)
		, m_cell_value_arena(1024)
	{

	}

	typed_matrix::typed_matrix(memory::arena* header_arena, const std::vector<column_header>& columns, const std::vector<std::string>& shared_string_table, const std::vector<std::uint32_t>& cell_strs, const std::unordered_map<int, std::uint16_t>& row_indexes)
		: m_row_sz(row_indexes.size())
		, m_column_sz(columns.size())
		, m_columns(columns)
		, m_is_str_key(false)
		, m_column_indexes(init_column_indexes(columns))
		, m_shared_str_table(shared_string_table)
		, m_cell_strs(cell_strs)
		, m_int_row_indexes(row_indexes)
		, m_cell_values(row_indexes.size())
		, m_header_arena(header_arena)
		, m_cell_value_arena(1024)
	{

	}

	typed_row typed_matrix::get_row(const std::string& cur_row_key)
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
	typed_row typed_matrix::get_row(const int& cur_row_key)
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
	const container::arena_typed_value* typed_matrix::get_cell(const typed_row& row_idx, const typed_matrix::column_index col_idx)
	{
		if (row_idx.m_matrix != this)
		{
			return nullptr;
		}
		if (row_idx.m_row_index == 0)
		{
			return nullptr;
		}
		if (!col_idx.valid())
		{
			return nullptr;
		}
		std::uint16_t cur_row_idx = row_idx.m_row_index - 1;
		std::uint16_t cur_column_idx = col_idx.value() - 1;
		if (cur_row_idx >= m_row_sz)
		{
			return nullptr;
		}
		if (cur_column_idx >= m_column_sz)
		{
			return nullptr;
		}
		return get_cell_safe(cur_row_idx, cur_column_idx);
	}
	const std::string& typed_matrix::get_cell_str(const typed_row& row_idx, typed_matrix::column_index col_idx) const
	{
		if (row_idx.m_matrix != this)
		{
			return m_shared_str_table[0];
		}
		if (row_idx.m_row_index == 0)
		{
			return m_shared_str_table[0];
		}
		if (!col_idx.valid())
		{
			return m_shared_str_table[0];
		}
		std::uint16_t cur_row_idx = row_idx.m_row_index - 1;
		std::uint16_t cur_column_idx = col_idx.value() - 1;
		if (cur_row_idx >= m_row_sz)
		{
			return m_shared_str_table[0];
		}
		if (cur_column_idx >= m_column_sz)
		{
			return m_shared_str_table[0];
		}
		auto cur_sst_idx = m_cell_strs[std::uint32_t(cur_row_idx) * m_column_sz + cur_column_idx];
		return m_shared_str_table[cur_sst_idx];

	}
	const container::arena_typed_value* typed_matrix::get_cell(const typed_row& row_idx, const std::string& cur_column_key)
	{
		if (row_idx.m_matrix != this)
		{
			return nullptr;
		}
		if (row_idx.m_row_index == 0)
		{
			return nullptr;
		}
		auto cur_iter = m_column_indexes.find(cur_column_key);
		if (cur_iter == m_column_indexes.end())
		{
			return nullptr;
		}
		std::uint16_t cur_row_idx = row_idx.m_row_index - 1;
		std::uint16_t cur_column_idx = cur_iter->second;
		if (cur_row_idx >= m_row_sz)
		{
			return nullptr;
		}

		return get_cell_safe(cur_row_idx, cur_column_idx);
	}

	const container::arena_typed_value* typed_matrix::get_cell_safe(std::uint16_t row_idx, std::uint16_t column_idx)
	{
		m_read_counter++;

		if (m_cell_values[row_idx].empty())
		{
			m_cell_values[row_idx] = std::vector< const container::arena_typed_value*>(m_column_sz);
		}
		if (m_cell_values[row_idx][column_idx])
		{
			return m_cell_values[row_idx][column_idx];
		}
		else
		{
			auto cur_sst_idx = m_cell_strs[std::uint32_t(row_idx) * m_column_sz + column_idx];
			if (cur_sst_idx == 0)
			{
				// empty str;
				return nullptr;
			}
			else
			{
				auto cur_parser = container::arena_typed_string_parser(m_cell_value_arena);
				auto cur_cell_value = cur_parser.parse_value_with_type(m_columns[column_idx].type_desc, m_shared_str_table[cur_sst_idx]);
				m_cell_values[row_idx][column_idx] = cur_cell_value;
				return cur_cell_value;
			}
		}
	}
	void typed_matrix::drop_cache()
	{
		m_read_counter = 0;
		for (auto& one_vec : m_cell_values)
		{
			one_vec.clear();
			one_vec.shrink_to_fit();
		}
		m_cell_value_arena.drop();
	}
	typed_matrix::~typed_matrix()
	{
		drop_cache();
		delete m_header_arena;
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

		std::vector<std::vector<std::uint32_t>> cell_sst_indexes(m_row_sz, std::vector<std::uint32_t>(m_column_sz));
		for (int i = 0; i < m_row_sz; i++)
		{
			for (int j = 0; j < m_column_sz; j++)
			{
				cell_sst_indexes[i][j] = m_cell_strs[i * m_column_sz + j];
			}
		}
		result["headers"] = headers;
		result["shared_str_table"] = m_shared_str_table;
		result["row_matrix"] = cell_sst_indexes;
		return result;
		

	}

	typed_matrix* typed_matrix::from_json(const json& json_matrix)
	{
		std::vector<std::array<std::string, 3>> headers;
		std::vector<std::string> shared_string_table;
		std::vector<std::vector<std::uint32_t>> cells;
		try
		{
			json_matrix.at("headers").get_to(headers);
			json_matrix.at("shared_str_table").get_to(shared_string_table);
			json_matrix.at("row_matrix").get_to(cells);
		}
		catch (std::exception& e)
		{
			return nullptr;
		}
		std::vector<input_header> cur_input_header(headers.size());
		for (int i = 0; i < headers.size(); i++)
		{
			cur_input_header[i].name = headers[i][0];
			cur_input_header[i].comment = headers[i][1];
			cur_input_header[i].type_str = headers[i][2];
		}
		return construct(cur_input_header, shared_string_table, cells);
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
	const container::arena_typed_value* typed_row::get_cell(typed_matrix::column_index column_idx) const
	{
		if (!m_matrix)
		{
			return nullptr;
		}
		return m_matrix->get_cell(*this, column_idx);
	}
	const container::arena_typed_value* typed_row::get_cell(const std::string& cur_column_key) const
	{
		if (!m_matrix)
		{
			return nullptr;
		}
		return m_matrix->get_cell(*this, cur_column_key);
	}
	bool typed_row::valid() const
	{
		return m_matrix && m_row_index;
	}
	typed_row::typed_row(typed_matrix* matrix, std::uint16_t row_index)
		: m_matrix(matrix)
		, m_row_index(row_index)
	{

	}
	typed_row typed_matrix::begin_row()
	{
		return typed_row(this, 1);
	}

	typed_row typed_matrix::next_row(const typed_row& pre_row)
	{
		if (!pre_row.valid())
		{
			return {};
		}
		if (pre_row.m_matrix != this)
		{
			return {};
		}
		if (pre_row.m_row_index + 1 == m_cell_values.size())
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
		if (col_idx.value() >= m_columns.size())
		{
			return nullptr;
		}
		return &m_columns[col_idx.value() - 1];
	}

}