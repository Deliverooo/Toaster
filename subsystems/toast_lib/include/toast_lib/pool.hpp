#pragma once

#include <mutex>
#include <ranges>
#include <shared_mutex>
#include <vector>

#include "handle.hpp"
#include "toast_assert.h"

namespace toaster
{
	template<typename TData>
	class Pool
	{
	public:
		using DestructorFn = void(*)(TData *, void *);
		using HandleType   = Handle<TData>;

		Pool() = default;

		explicit Pool(DestructorFn p_destructor_fn, void *p_user_data) : m_destructorFn(p_destructor_fn), m_destructorUserData(p_user_data)
		{
		}

		~Pool() { clear(); }

		auto setDestructorFn(DestructorFn p_destructor_fn) { m_destructorFn = p_destructor_fn; }
		auto setDestructorUserData(void *p_user_data) { m_destructorUserData = p_user_data; }

		template<typename... TArgs>
		auto emplace(TArgs &&... p_args) -> HandleType
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};

			uint32 id{0u};
			uint32 magic{1u};

			if (!m_freeIndices.empty())
			{
				id = m_freeIndices.back();
				m_freeIndices.pop_back();
				magic = m_entries[id]->magic + 1;
			}
			else
			{
				id = m_entries.size();
				m_entries.resize(id + 1u);
				m_entries[id] = new Entry{};
			}

			m_entries[id]->data  = TData{std::forward<TArgs>(p_args)...};
			m_entries[id]->magic = magic;
			m_entries[id]->alive = true;

			return HandleType{id, magic};
		}

		auto destroy(HandleType p_handle) -> void
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};

			if (!_isValid(p_handle))
				TST_PERMA_ASSERT(false);

			uint32 id{p_handle.getId()};

			m_entries[id]->alive = false;
			++m_entries[id]->magic;
			m_freeIndices.push_back(id);

			if (m_destructorFn)
				m_destructorFn(std::addressof(m_entries[id]->data), m_destructorUserData);
		}

		auto clear() -> void
		{
			for (auto &entry: m_entries)
			{
				if (m_destructorFn && entry->alive)
					m_destructorFn(&entry->data, m_destructorUserData);
				delete entry;
			}
			m_entries.clear();
			m_freeIndices.clear();
		}

		auto tryGet(HandleType p_handle) -> TData *
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			if (!_isValid(p_handle))
				return nullptr;
			return std::addressof(m_entries[p_handle.getId()]->data);
		}

		auto tryGet(HandleType p_handle) const -> const TData *
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			if (!_isValid(p_handle))
				return nullptr;
			return std::addressof(m_entries[p_handle.getId()]->data);
		}

		auto operator[](HandleType p_handle) -> TData &
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			TData &                             data{m_entries[p_handle.getId()]->data};
			return data;
		}

		auto operator[](HandleType p_handle) const -> const TData &
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			TData &                             data{m_entries[p_handle.getId()]->data};
			return data;
		}

		auto isValid(HandleType p_handle) const -> bool
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return _isValid(p_handle);
		}

	private:
		auto _isValid(HandleType p_handle) const -> bool
		{
			uint32 id{p_handle.getId()};

			if (id >= m_entries.size())
				return false;

			return m_entries[id]->alive && (m_entries[id]->magic == p_handle.getMagic());
		}

		struct Entry
		{
			TData  data;
			uint32 magic{0u};
			bool   alive{false};
		};

		mutable std::shared_mutex m_mutex;

		std::vector<Entry *> m_entries;
		std::vector<uint32>  m_freeIndices;

		DestructorFn m_destructorFn{nullptr};
		void *       m_destructorUserData{nullptr};
	};
}
