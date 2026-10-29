#pragma once

#include <threads.h>

#include "handle.hpp"
#include "toast_assert.h"
#include "vector.hpp"

namespace toaster
{
	template<typename TData, uint64 PageSize = 64u> // 64 Elements, not bytes
	class Pool
	{
	public:
		using DestructorFn = void(*)(TData *, void *);
		using HandleType   = Handle<TData>;

		Pool()
		{
			mtx_init(&m_mtx, mtx_plain);
		}

		explicit Pool(DestructorFn p_destructor_fn, void *p_user_data) : m_destructorFn(p_destructor_fn), m_destructorUserData(p_user_data)
		{
			mtx_init(&m_mtx, mtx_plain);
		}

		~Pool()
		{
			clear();
			mtx_destroy(&m_mtx);
		}

		Pool(const Pool &)            = delete;
		Pool &operator=(const Pool &) = delete;

		auto setDestructorFn(DestructorFn p_destructor_fn) { m_destructorFn = p_destructor_fn; }
		auto setDestructorUserData(void *p_user_data) { m_destructorUserData = p_user_data; }

		template<typename... TArgs>
		auto emplace(TArgs &&... p_args) -> HandleType
		{
			mtx_lock(&m_mtx);

			uint32 id{0u};
			uint32 magic{1u};

			if (!m_freeIndices.empty())
			{
				id = m_freeIndices.back();
				m_freeIndices.pop_back();

				Entry &entry{_getEntry(id)};
				magic = entry.magic + 1u;
			}
			else
			{
				id = m_totalSlots;
				m_totalSlots++;

				uint32 required_pages{(id / static_cast<uint32>(PageSize)) + 1u};
				if (required_pages > m_pages.size())
					m_pages.push_back(new Entry[PageSize]());
			}

			Entry &entry{_getEntry(id)};
			new(static_cast<void *>(&entry.data)) TData{std::forward<TArgs>(p_args)...};
			entry.magic = magic;
			entry.alive = true;

			HandleType out_handle{id, magic};
			mtx_unlock(&m_mtx);

			return out_handle;
		}

		auto destroy(HandleType p_handle) -> void
		{
			mtx_lock(&m_mtx);

			if (!_isValid(p_handle))
			{
				mtx_unlock(&m_mtx);
				TST_PERMA_ASSERT(false);
			}

			uint32 id{p_handle.getId()};
			_destroy(id);

			mtx_unlock(&m_mtx);
		}

		auto clear() -> void
		{
			mtx_lock(&m_mtx);

			for (uint64 i{0ull}; i < m_totalSlots; ++i)
			{
				Entry &entry{_getEntry(i)};
				if (m_destructorFn && entry.alive)
					m_destructorFn(&entry.data, m_destructorUserData);
			}

			for (Entry *page: m_pages)
				delete[] page;
			m_pages.clear();

			m_totalSlots = 0ull;

			mtx_unlock(&m_mtx);
		}

		auto tryGet(HandleType p_handle) -> TData *
		{
			mtx_lock(&m_mtx);
			if (!_isValid(p_handle))
			{
				mtx_unlock(&m_mtx);
				return nullptr;
			}

			TData *data{std::addressof(_getEntry(p_handle.getId()).data)};
			mtx_unlock(&m_mtx);

			return data;
		}

		auto tryGet(HandleType p_handle) const -> const TData *
		{
			mtx_lock(&m_mtx);
			if (!_isValid(p_handle))
			{
				mtx_unlock(&m_mtx);
				return nullptr;
			}

			const TData *data{std::addressof(_getEntry(p_handle.getId()).data)};
			mtx_unlock(&m_mtx);

			return data;
		}

		auto operator[](HandleType p_handle) -> TData &
		{
			mtx_lock(&m_mtx);
			TData &data{_getEntry(p_handle.getId()).data};
			mtx_unlock(&m_mtx);
			return data;
		}

		auto operator[](HandleType p_handle) const -> const TData &
		{
			mtx_lock(&m_mtx);
			const TData &data{_getEntry(p_handle.getId()).data};
			mtx_unlock(&m_mtx);
			return data;
		}

		auto isValid(HandleType p_handle) const -> bool
		{
			mtx_lock(&m_mtx);
			const bool valid{_isValid(p_handle)};
			mtx_unlock(&m_mtx);
			return valid;
		}

		auto removeAt(uint32 p_id) -> void
		{
			mtx_lock(&m_mtx);

			if (p_id >= m_totalSlots)
			{
				mtx_unlock(&m_mtx);
				TST_PERMA_ASSERT(false);
			}

			_destroy(p_id);

			mtx_unlock(&m_mtx);
		}

		template<typename TFunc>
		auto forEachAlive(TFunc &&p_func) -> void
		{
			mtx_lock(&m_mtx);

			for (uint64 i{0ull}; i < m_totalSlots; ++i)
			{
				Entry &entry{_getEntry(i)};
				if (entry.alive)
					p_func(entry.data, i);
			}

			mtx_unlock(&m_mtx);
		}

		auto getLiveCount() -> uint32
		{
			mtx_lock(&m_mtx);

			uint32 alive_count{0u};
			for (uint64 i{0ull}; i < m_totalSlots; ++i)
			{
				if (_getEntry(i).alive)
					++alive_count;
			}
			mtx_unlock(&m_mtx);

			return alive_count;
		}

	private:
		struct Entry
		{
			TData  data;
			uint32 magic{0u};
			bool   alive{false};
		};

		auto _getEntry(uint32 p_id) -> Entry &
		{
			return m_pages[p_id / static_cast<uint32>(PageSize)][p_id % static_cast<uint32>(PageSize)];
		}

		auto _getEntry(uint32 p_id) const -> const Entry &
		{
			return m_pages[p_id / static_cast<uint32>(PageSize)][p_id % static_cast<uint32>(PageSize)];
		}

		auto _isValid(HandleType p_handle) const -> bool
		{
			uint32 id{p_handle.getId()};

			if (id >= m_totalSlots)
				return false;

			const Entry &entry{_getEntry(id)};
			return entry.alive && (entry.magic == p_handle.getMagic());
		}

		auto _destroy(uint32 p_id) -> void
		{
			Entry &entry{_getEntry(p_id)};
			entry.alive = false;
			++entry.magic;

			m_freeIndices.push_back(p_id);

			if (m_destructorFn)
				m_destructorFn(std::addressof(entry.data), m_destructorUserData);

			entry.data.~TData();
		}

		mutable mtx_t m_mtx;

		Vector<Entry *> m_pages;
		Vector<uint32>  m_freeIndices;

		DestructorFn m_destructorFn{nullptr};
		void *       m_destructorUserData{nullptr};

		uint64 m_totalSlots{0ull};
	};
}
