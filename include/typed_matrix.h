#pragma once
#include <unordered_map>
#include <typed_string/arena_typed_string.h>

namespace spiritsaway::typed_matrix
{
	struct matrix_header
	{
		std::string name;
		std::string comment;
		container::typed_string_desc* desc;
	};
	class typed_matrix
	{
		struct cached_cell_value
		{
			const container::arena_typed_value* value;
			std::uint16_t counter = 0;
			std::uint16_t row_idx = 0;
			std::uint16_t column_ix = 0;
		};
		struct cell_value
		{
			std::uint32_t shared_str_idx;
			const cached_cell_value* cached_value;
		};
	public:
		const std::uint16_t m_row_sz;
		const std::uint16_t m_column_sz;
		const bool m_is_str_key;
		const std::vector<matrix_header> m_columns;
		const std::unordered_map<std::string, std::uint16_t> m_column_indexes;
		const std::unordered_map<std::string, std::uint16_t> m_str_row_indexes;
		const std::unordered_map<int, std::uint16_t> m_int_row_indexes;
		const std::vector<std::string> m_shared_str_table;
		const std::vector<std::uint32_t> m_cell_strs;
	private:
		std::vector<std::vector<cached_cell_value>> m_cell_values;
		typed_matrix(const std::vector<matrix_header>& columns, const std::vector<std::string>& shared_string_table, const std::vector<std::uint32_t>& cell_strs, const std::unordered_map<std::string, std::uint16_t>& row_indexes);
		typed_matrix(const std::vector<matrix_header>& columns, const std::vector<std::string>& shared_string_table, const std::vector<std::uint32_t>& cell_strs, const std::unordered_map<int, std::uint16_t>& row_indexes);
		const container::arena_typed_value* get_cell_safe(std::uint16_t row_idx, std::uint16_t column_idx);
	public:
		static std::unordered_map<std::string, std::uint16_t> init_column_indexes(const std::vector<matrix_header>& in_columns);
		static typed_matrix* construct(const std::vector<matrix_header>& headers, const std::vector<std::string>& shared_string_table, const std::vector<std::vector<std::uint32_t>>& row_values);

		bool get_row_idx(const std::string& cur_row_key, std::uint16_t& result) const;
		bool get_row_idx(const int& cur_row_key, std::uint16_t& result) const;
		bool get_column_idx(const std::string& cur_column_key, std::uint16_t& result) const;
		const container::arena_typed_value* get_cell(std::uint16_t row_idx, std::uint16_t column_idx);
		const container::arena_typed_value* get_cell(int row_key, const std::string& column_key);
		const container::arena_typed_value* get_cell(const std::string& row_key, const std::string& column_key);
	};
}