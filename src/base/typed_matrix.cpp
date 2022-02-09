#include "typed_matrix.h"
#include <iostream>

namespace spiritsaway::typed_matrix
{
	typed_matrix* typed_matrix::construct(const std::vector<matrix_header>& headers, const std::vector<std::string>& shared_string_table, const std::vector<std::vector<std::uint32_t>>& row_values)
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
		if (!headers[0].desc)
		{
			std::cout << "head desc empty" << std::endl;
			return nullptr;
		}
		auto cur_row_key_type = headers[0].desc->_type;
		if (cur_row_key_type != container::basic_value_type::number_32 && cur_row_key_type != container::basic_value_type::string)
		{
			std::cout << "invalid row key type " << headers[0].desc->to_string() << std::endl;
			return nullptr;
		}

		std::uint16_t row_sz = row_values.size();
		std::uint16_t column_sz = headers.size();
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
			auto cur_matrix = new typed_matrix(headers, new_sst, cell_strs, int_row_indexes);
			return cur_matrix;
		}
		else
		{
			auto cur_matrix = new typed_matrix(headers, new_sst, cell_strs, str_row_indexes);
			return cur_matrix;
		}

	}
	typed_matrix::typed_matrix(const std::vector<matrix_header>& columns, const std::vector<std::string>& shared_string_table, const std::vector<std::uint32_t>& cell_strs, const std::unordered_map<std::string, std::uint16_t>& row_indexes)
		: m_row_sz(row_indexes.size())
		, m_column_sz(columns.size())
		, m_is_str_key(true)
		, m_column_indexes(init_column_indexes(columns))
		, m_shared_str_table(shared_string_table)
		, m_cell_strs(cell_strs)
		, m_str_row_indexes(row_indexes)
		, m_cell_values(row_indexes.size())
	{

	}

	typed_matrix::typed_matrix(const std::vector<matrix_header>& columns, const std::vector<std::string>& shared_string_table, const std::vector<std::uint32_t>& cell_strs, const std::unordered_map<int, std::uint16_t>& row_indexes)
		: m_row_sz(row_indexes.size())
		, m_column_sz(columns.size())
		, m_is_str_key(false)
		, m_column_indexes(init_column_indexes(columns))
		, m_shared_str_table(shared_string_table)
		, m_cell_strs(cell_strs)
		, m_int_row_indexes(row_indexes)
		, m_cell_values(row_indexes.size())
	{

	}

	bool typed_matrix::get_row_idx(const std::string& cur_row_key, std::uint16_t& result) const
	{
		if (!m_is_str_key)
		{
			return false;
		}
		auto cur_iter = m_str_row_indexes.find(cur_row_key);
		if (cur_iter == m_str_row_indexes.end())
		{
			return false;
		}
		result = cur_iter->second;
		return true;
	}
	bool typed_matrix::get_row_idx(const int& cur_row_key, std::uint16_t& result) const
	{
		if (m_is_str_key)
		{
			return false;
		}
		auto cur_iter = m_int_row_indexes.find(cur_row_key);
		if (cur_iter == m_int_row_indexes.end())
		{
			return false;
		}
		result = cur_iter->second;
		return true;
	}
	bool typed_matrix::get_column_idx(const std::string& cur_column_key, std::uint16_t& result) const
	{
		auto cur_iter = m_column_indexes.find(cur_column_key);
		if (cur_iter == m_column_indexes.end())
		{
			return false;
		}
		result = cur_iter->second;
		return true;
	}
	const container::arena_typed_value* typed_matrix::get_cell(int row_key, const std::string& column_key)
	{
		std::uint16_t cur_row_idx = 0;
		if (!get_row_idx(row_key, cur_row_idx))
		{
			return nullptr;
		}
		std::uint16_t cur_column_idx = 0;
		if (!get_column_idx(column_key, cur_column_idx))
		{
			return nullptr;
		}
		return get_cell_safe(cur_row_idx, cur_column_idx);
	}
	const container::arena_typed_value* typed_matrix::get_cell(const std::string& row_key, const std::string& column_key)
	{
		std::uint16_t cur_row_idx = 0;
		if (!get_row_idx(row_key, cur_row_idx))
		{
			return nullptr;
		}
		std::uint16_t cur_column_idx = 0;
		if (!get_column_idx(column_key, cur_column_idx))
		{
			return nullptr;
		}
		return get_cell_safe(cur_row_idx, cur_column_idx);
	}

	const container::arena_typed_value* typed_matrix::get_cell(std::uint16_t row_idx, std::uint16_t column_idx)
	{
		if (row_idx >= m_row_sz)
		{
			return nullptr;
		}
		if (column_idx >= m_column_sz)
		{
			return nullptr;
		}
		return get_cell_safe(row_idx, column_idx);
	}
	const container::arena_typed_value* typed_matrix::get_cell_safe(std::uint16_t row_idx, std::uint16_t column_idx)
	{
		if (m_cell_values[row_idx].empty())
		{
			m_cell_values[row_idx] = std::vector< cached_cell_value>(m_column_sz);
		}
		if (m_cell_values[row_idx][column_idx].value)
		{
			m_cell_values[row_idx][column_idx].counter++;
			return m_cell_values[row_idx][column_idx].value;
		}
		else
		{
			if (m_cell_strs[row_idx * m_column_sz + column_idx] == 0)
			{
				// empty str;
				return nullptr;
			}
			else
			{

			}
		}
	}
}