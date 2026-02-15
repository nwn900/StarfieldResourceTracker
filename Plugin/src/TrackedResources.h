#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_set>

namespace ResourceTracker
{
	// Form ID type (matches RE::FormID / game form identifier).
	using FormID = std::uint32_t;

	// Persistent set of form IDs the player has tagged as "needed" (missing for research / weapon mod / armor mod / industrial).
	// When an item with one of these form IDs appears in vendor menu, container, or selection, show magnifier icon.
	class TrackedResources
	{
	public:
		static TrackedResources& Get();

		void Add(FormID a_formId);
		void Remove(FormID a_formId);
		bool Contains(FormID a_formId) const;
		void Clear();

		// Persist to Data/SFSE/Plugins/ResourceTracker.json. Call on add/remove and game save.
		void Load();
		void Save();

		// For UI hooks: call when drawing an item entry; returns true if a magnifier icon should be shown.
		inline static bool ShouldShowMagnifier(FormID a_formId)
		{
			return Get().Contains(a_formId);
		}

	private:
		TrackedResources() = default;
		~TrackedResources() = default;
		TrackedResources(const TrackedResources&) = delete;
		TrackedResources& operator=(const TrackedResources&) = delete;

		mutable std::mutex _mtx;
		std::unordered_set<FormID> _formIds;
		static constexpr const char* _filename = "ResourceTracker.json";
	};
}
