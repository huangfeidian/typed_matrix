#pragma once
#include <unordered_map>
#include <typed_string/typed_string_desc.h>
#include <any_container/decode.h>
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
		std::shared_ptr< const container::typed_string_desc> type_desc;
	};
	class typed_row;

	class typed_matrix
	{
	public:
		class column_index
		{
			std::uint16_t m_value;
		public:
			column_index()
				: m_value(0)
			{

			}
			column_index(const column_index& other)
				: m_value(other.m_value)
			{

			}
			column_index& operator=(const column_index& other)
			{
				m_value = other.m_value;
				return *this;
			}
		private:
			friend class typed_matrix;
			void set_value(std::uint16_t new_value)
			{
				m_value = new_value;
			}
		public:
			std::uint16_t value() const
			{
				return m_value;
			}
			bool valid() const
			{
				return m_value != 0;
			}
		};
	private:
		typed_matrix(const std::vector<column_header>& columns, const std::vector<json>& shared_json_table, const std::vector<std::vector<std::uint32_t>>& cell_value_indexes, const std::unordered_map<std::string, std::uint16_t>& row_indexes);
		typed_matrix(const std::vector<column_header>& columns, const std::vector<json>& shared_json_table, const std::vector<std::vector<std::uint32_t>>& cell_value_indexes, const std::unordered_map<std::uint32_t, std::uint16_t>& row_indexes);
		const json& get_cell_safe(const std::uint16_t& row_idx, const std::uint16_t col_idx) const;
	public:
		const std::uint16_t m_row_sz;
		const std::uint16_t m_column_sz;
		const bool m_is_str_key;
		const std::vector<column_header> m_columns;
		const std::unordered_map<std::string, std::uint16_t> m_column_indexes;
		const std::unordered_map<std::string, std::uint16_t> m_str_row_indexes;
		const std::unordered_map<std::uint32_t, std::uint16_t> m_int_row_indexes;
		const std::vector<json> m_cell_json_values;
		const std::vector<std::vector<std::uint32_t>> m_cell_value_indexes;

	public:
		static std::unordered_map<std::string, std::uint16_t> init_column_indexes(const std::vector<column_header>& in_columns);
		static typed_matrix* construct(const std::vector<input_header>& headers, const std::vector<json>& shared_json_table, const std::vector<std::vector<std::uint32_t>>& row_values);

		typed_row get_row(const std::string& cur_row_key) const;
		typed_row get_row(const std::uint32_t& cur_row_key) const;
		column_index get_column_idx(const std::string& cur_column_key) const;
		
		const json& get_cell(const typed_row& row_idx, column_index col_idx) const;
		const json& get_cell(const typed_row& row_idx, const std::string& cur_column_key) const;
		template <typename T>
		bool get_cell(const typed_row& row_idx, column_index col_idx, T& dest) const
		{
			auto cur_cell_v = get_cell(row_idx, col_idx);
			if (!cur_cell_v)
			{
				return false;
			}
			
			return serialize::decode(cur_cell_v, dest);
		}
		typed_row begin_row() const;
		typed_row next_row(const typed_row& pre_row) const;
		column_index begin_column() const;
		column_index next_column(column_index pre_column) const;
		const column_header* get_column_header(column_index col_idx) const;
		~typed_matrix();
		json to_json() const;
		static typed_matrix* from_json(const json& json_matrix);
	};

	class typed_row
	{
		const typed_matrix* m_matrix;
		std::uint16_t m_row_index;
		friend class typed_matrix;
		typed_row(const typed_matrix* matrix, std::uint16_t row_index);
	public:
		bool valid() const;
		typed_row();
		const typed_matrix* matrix() const
		{
			return m_matrix;
		}
		typed_row(const typed_row& other) = default;
		typed_row& operator=(const typed_row& other) = default;
		typed_matrix::column_index get_column_idx(const std::string& cur_column_key) const;
		const json& get_cell(typed_matrix::column_index column_idx) const;
		const json& get_cell(const std::string& cur_column_key) const;
		std::uint16_t row_index() const
		{
			return m_row_index;
		}
		template <typename T, typename K>
		bool expect_value(const K&  key_or_idx, T& dest)
		{
			auto cur_cell_v = get_cell(key_or_idx);
			if (!cur_cell_v)
			{
				return false;
			}
			return serialize::decode(cur_cell_v, dest);
		}
	};
}
