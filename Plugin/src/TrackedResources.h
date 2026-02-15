#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace ResourceTracker
{
	using FormID = std::uint32_t;

	class TrackedResources
	{
	public:
		static TrackedResources& Get();

		void Add(FormID a_formId);
		void AddBulk(const std::vector<FormID>& a_ids);
		void Remove(FormID a_formId);
		bool Contains(FormID a_formId) const;
		void Clear();
		std::size_t Count() const;
		std::vector<FormID> GetAll() const;

		void Load();
		void Save() const;

	private:
		TrackedResources() = default;
		~TrackedResources() = default;
		TrackedResources(const TrackedResources&) = delete;
		TrackedResources& operator=(const TrackedResources&) = delete;

		void SaveInternal() const;

		mutable std::recursive_mutex _mtx;
		std::unordered_set<FormID> _formIds;
		static constexpr const char* _filename = "ResourceTracker.json";
	};
}
