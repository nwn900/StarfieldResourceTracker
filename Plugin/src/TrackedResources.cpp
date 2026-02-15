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
		Save();
	}

	void TrackedResources::Remove(FormID a_formId)
	{
		std::lock_guard lk(_mtx);
		_formIds.erase(a_formId);
		Save();
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
		Save();
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

	void TrackedResources::Save()
	{
		// Create Data, then Data\\SFSE, then Data\\SFSE\\Plugins
		for (const char* sub : { "Data", "Data\\SFSE", "Data\\SFSE\\Plugins" })
			CreateDirectoryA(sub, nullptr);
		const std::string dir = "Data\\SFSE\\Plugins";
		const std::string path = dir + "\\" + _filename;

		std::vector<std::uint32_t> ids;
		{
			std::lock_guard lk(_mtx);
			ids.reserve(_formIds.size());
			for (FormID id : _formIds)
				ids.push_back(static_cast<std::uint32_t>(id));
		}

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
