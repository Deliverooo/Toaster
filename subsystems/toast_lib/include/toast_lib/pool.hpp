#pragma once

#include <ranges>
#include <vector>

#include "handle.hpp"
#include "toast_assert.h"

namespace toaster
{
	template<typename TData>
	class Pool
	{
	public:
		using DestructorFn = void(*)(TData *, void*);
		using HandleType   = Handle<TData>;

		Pool() = default;

		explicit Pool(DestructorFn p_destructor_fn, void *p_user_data) : m_destructorFn(p_destructor_fn), m_destructorUserData(p_user_data)
		{
		}

		auto setDestructorFn(DestructorFn p_destructor_fn) { m_destructorFn = p_destructor_fn; }
		auto setDestructorUserData(void *p_user_data) { m_destructorUserData = p_user_data; }

		template<typename... TArgs>
		auto emplace(TArgs &&... p_args) -> HandleType
		{
			uint32 id{0u};
			uint32 magic{1u};

			if (!m_freeIndices.empty())
			{
				id = m_freeIndices.back();
				m_freeIndices.pop_back();
				magic = m_entries[id].magic + 1;
			}
			else
			{
				id = m_entries.size();
				m_entries.resize(id + 1u);
			}

			m_entries[id].data  = TData{std::forward<TArgs>(p_args)...};
			m_entries[id].magic = magic;
			m_entries[id].alive = true;

			return HandleType{id, magic};
		}

		auto destroy(HandleType p_handle) -> void
		{
			if (!isValid(p_handle))
				TST_PERMA_ASSERT(false);
			// return;

			uint32 id{p_handle.getId()};
			m_entries[id].alive = false;
			++m_entries[id].magic;
			m_freeIndices.push_back(id);

			if (m_destructorFn)
				m_destructorFn(std::addressof(m_entries[id].data), m_destructorUserData);
		}

		auto clear() -> void
		{
			if (m_destructorFn)
			{
				for (auto &entry: m_entries)
					if (entry.alive)
						m_destructorFn(&entry.data, m_destructorUserData);
			}
			m_entries.clear();
			m_freeIndices.clear();
		}

		auto tryGet(HandleType p_handle) -> TData *
		{
			if (!isValid(p_handle))
				return nullptr;
			return std::addressof(m_entries[p_handle.getId()].data);
		}

		auto tryGet(HandleType p_handle) const -> const TData *
		{
			if (!isValid(p_handle))
				return nullptr;
			return std::addressof(m_entries[p_handle.getId()].data);
		}

		auto operator[](HandleType p_handle) -> TData & { return m_entries[p_handle.getId()].data; }
		auto operator[](HandleType p_handle) const -> const TData & { return m_entries[p_handle.getId()].data; }

		auto isValid(HandleType p_handle) const -> bool
		{
			uint32 id{p_handle.getId()};

			if (id >= m_entries.size())
				return false;

			return m_entries[id].alive && (m_entries[id].magic == p_handle.getMagic());
		}

	private:
		struct Entry
		{
			TData  data;
			uint32 magic{0u};
			bool   alive{false};
		};

		std::vector<Entry>  m_entries;
		std::vector<uint32> m_freeIndices;

		DestructorFn m_destructorFn{nullptr};
		void *       m_destructorUserData{nullptr};
	};
}
