#pragma once

#include <threads.h>
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

		Pool()
		{
			mtx_init(&m_mutex, mtx_plain);
		}

		explicit Pool(DestructorFn p_destructor_fn, void *p_user_data) : m_destructorFn(p_destructor_fn), m_destructorUserData(p_user_data)
		{
			mtx_init(&m_mutex, mtx_plain);
		}

		~Pool()
		{
			clear();
			mtx_destroy(&m_mutex);
		}

		Pool(const Pool &)            = delete;
		Pool &operator=(const Pool &) = delete;

		auto setDestructorFn(DestructorFn p_destructor_fn) { m_destructorFn = p_destructor_fn; }
		auto setDestructorUserData(void *p_user_data) { m_destructorUserData = p_user_data; }

		template<typename... TArgs>
		auto emplace(TArgs &&... p_args) -> HandleType
		{
			mtx_lock(&m_mutex);

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

			HandleType out_handle{id, magic};
			mtx_unlock(&m_mutex);

			return out_handle;
		}

		auto destroy(HandleType p_handle) -> void
		{
			mtx_lock(&m_mutex);

			if (!_isValid(p_handle))
			{
				mtx_unlock(&m_mutex);
				TST_PERMA_ASSERT(false);
			}

			uint32 id{p_handle.getId()};

			m_entries[id]->alive = false;
			++m_entries[id]->magic;
			m_freeIndices.push_back(id);

			if (m_destructorFn)
				m_destructorFn(std::addressof(m_entries[id]->data), m_destructorUserData);

			mtx_unlock(&m_mutex);
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
			mtx_lock(&m_mutex);
			if (!_isValid(p_handle))
			{
				mtx_unlock(&m_mutex);
				return nullptr;
			}

			TData *data{std::addressof(m_entries[p_handle.getId()]->data)};
			mtx_unlock(&m_mutex);

			return data;
		}

		auto tryGet(HandleType p_handle) const -> const TData *
		{
			mtx_lock(&m_mutex);
			if (!_isValid(p_handle))
			{
				mtx_unlock(&m_mutex);
				return nullptr;
			}

			const TData *data{std::addressof(m_entries[p_handle.getId()]->data)};
			mtx_unlock(&m_mutex);

			return data;
		}

		auto operator[](HandleType p_handle) -> TData &
		{
			mtx_lock(&m_mutex);
			TData &data{m_entries[p_handle.getId()]->data};
			mtx_unlock(&m_mutex);
			return data;
		}

		auto operator[](HandleType p_handle) const -> const TData &
		{
			mtx_lock(&m_mutex);
			TData &data{m_entries[p_handle.getId()]->data};
			mtx_unlock(&m_mutex);
			return data;
		}

		auto isValid(HandleType p_handle) const -> bool
		{
			mtx_lock(&m_mutex);
			const bool valid{_isValid(p_handle)};
			mtx_unlock(&m_mutex);
			return valid;
		}

		auto removeAt(uint32 p_id) -> void
		{
			mtx_lock(&m_mutex);

			if (p_id >= m_entries.size())
			{
				mtx_unlock(&m_mutex);

				TST_PERMA_ASSERT(false);
			}

			m_entries[p_id]->alive = false;
			++m_entries[p_id]->magic;
			m_freeIndices.push_back(p_id);

			if (m_destructorFn)
				m_destructorFn(std::addressof(m_entries[p_id]->data), m_destructorUserData);

			mtx_unlock(&m_mutex);
		}

		template<typename TFunc>
		auto forEachAlive(TFunc &&p_func) -> void
		{
			mtx_lock(&m_mutex);

			uint32 i{0u};
			for (auto &entry: m_entries)
			{
				if (entry->alive)
					p_func(entry->data, i);

				++i;
			}

			mtx_unlock(&m_mutex);
		}

		auto getLiveCount() -> uint32
		{
			mtx_lock(&m_mutex);

			uint32 alive_count{0u};
			for (auto &entry: m_entries)
				if (entry->alive)
					++alive_count;

			mtx_unlock(&m_mutex);

			return alive_count;
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

		mutable mtx_t m_mutex;

		std::vector<Entry *> m_entries;
		std::vector<uint32>  m_freeIndices;

		DestructorFn m_destructorFn{nullptr};
		void *       m_destructorUserData{nullptr};
	};
}
