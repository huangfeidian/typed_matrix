#pragma once
#include <unordered_map>
#include <typed_string/arena_typed_string.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace spiritsaway::typed_matrix
{
	struct input_header
	{
		std::string name;
		std::string comment;
		std::string type_str;
	};
	struct column_header : input_header
	{
		const container::typed_string_desc* type_desc;
	};
	class typed_matrix
	{

		memory::arena* m_header_arena = nullptr;
		memory::arena m_cell_value_arena;
	public:
		const std::uint16_t m_row_sz;
		const std::uint16_t m_column_sz;
		const bool m_is_str_key;
		const std::vector<column_header> m_columns;
		const std::unordered_map<std::string, std::uint16_t> m_column_indexes;
		const std::unordered_map<std::string, std::uint16_t> m_str_row_indexes;
		const std::unordered_map<int, std::uint16_t> m_int_row_indexes;
		const std::vector<std::string> m_shared_str_table;
		const std::vector<std::uint32_t> m_cell_strs;
	private:
		std::vector<std::vector<const container::arena_typed_value*>> m_cell_values;
		std::uint64_t m_read_counter = 0;
		typed_matrix(memory::arena* header_arena, const std::vector<column_header>& columns, const std::vector<std::string>& shared_string_table, const std::vector<std::uint32_t>& cell_strs, const std::unordered_map<std::string, std::uint16_t>& row_indexes);
		typed_matrix(memory::arena* header_arena, const std::vector<column_header>& columns, const std::vector<std::string>& shared_string_table, const std::vector<std::uint32_t>& cell_strs, const std::unordered_map<int, std::uint16_t>& row_indexes);
		const container::arena_typed_value* get_cell_safe(std::uint16_t row_idx, std::uint16_t column_idx);
	public:
		static std::unordered_map<std::string, std::uint16_t> init_column_indexes(const std::vector<column_header>& in_columns);
		static typed_matrix* construct(const std::vector<input_header>& headers, const std::vector<std::string>& shared_string_table, const std::vector<std::vector<std::uint32_t>>& row_values);

		bool get_row_idx(const std::string& cur_row_key, std::uint16_t& result) const;
		bool get_row_idx(const int& cur_row_key, std::uint16_t& result) const;
		bool get_column_idx(const std::string& cur_column_key, std::uint16_t& result) const;
		const container::arena_typed_value* get_cell(std::uint16_t row_idx, std::uint16_t column_idx);
		const container::arena_typed_value* get_cell(int row_key, const std::string& column_key);
		const container::arena_typed_value* get_cell(const std::string& row_key, const std::string& column_key);
		std::uint64_t read_counter() const
		{
			return m_read_counter;
		}
		void drop_cache();
		~typed_matrix();
		json to_json() const;
		static typed_matrix* from_json(const json& json_matrix);
	};
}