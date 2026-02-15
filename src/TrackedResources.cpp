#include "TrackedResources.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace ResourceTracker
{
	TrackedResources& TrackedResources::Get()
	{
		static TrackedResources instance;
		return instance;
	}

	void TrackedResources::Add(FormID a_formId)
	{
		std::lock_guard lk(_mtx);
		_formIds.insert(a_formId);
		SaveInternal();
	}

	void TrackedResources::AddBulk(const std::vector<FormID>& a_ids)
	{
		std::lock_guard lk(_mtx);
		for (auto id : a_ids)
			_formIds.insert(id);
		SaveInternal();
	}

	void TrackedResources::Remove(FormID a_formId)
	{
		std::lock_guard lk(_mtx);
		_formIds.erase(a_formId);
		SaveInternal();
	}

	bool TrackedResources::Contains(FormID a_formId) const
	{
		std::lock_guard lk(_mtx);
		return _formIds.count(a_formId) != 0;
	}

	void TrackedResources::Clear()
	{
		std::lock_guard lk(_mtx);
		_formIds.clear();
		SaveInternal();
	}

	std::size_t TrackedResources::Count() const
	{
		std::lock_guard lk(_mtx);
		return _formIds.size();
	}

	std::vector<FormID> TrackedResources::GetAll() const
	{
		std::lock_guard lk(_mtx);
		return { _formIds.begin(), _formIds.end() };
	}

	void TrackedResources::Load()
	{
		std::lock_guard lk(_mtx);
		_formIds.clear();

		const std::string path = std::string("Data\\SFSE\\Plugins\\") + _filename;
		std::ifstream f(path);
		if (!f)
			return;

		try
		{
			nlohmann::json j;
			f >> j;
			if (j.contains("tracked") && j["tracked"].is_array())
				for (const auto& v : j["tracked"])
					if (v.is_number_unsigned())
						_formIds.insert(static_cast<FormID>(v.get<std::uint32_t>()));
		}
		catch (...)
		{
			_formIds.clear();
		}
	}

	void TrackedResources::Save() const
	{
		std::lock_guard lk(_mtx);
		SaveInternal();
	}

	void TrackedResources::SaveInternal() const
	{
		for (const char* sub : { "Data", "Data\\SFSE", "Data\\SFSE\\Plugins" })
			CreateDirectoryA(sub, nullptr);

		const std::string path = std::string("Data\\SFSE\\Plugins\\") + _filename;

		std::vector<std::uint32_t> ids(_formIds.begin(), _formIds.end());

		try
		{
			nlohmann::json j;
			j["tracked"] = ids;
			std::ofstream f(path);
			if (f)
				f << j.dump(2);
		}
		catch (...) {}
	}
}
