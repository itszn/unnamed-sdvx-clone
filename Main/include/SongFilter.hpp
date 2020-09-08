#pragma once
#include "stdafx.h"
#include "SongSelect.hpp"
#include <Beatmap/MapDatabase.hpp>

enum FilterType
{
	All,
	Folder,
	Level,
	Collection
};

template<class ItemIndex>
class Filter
{
public:
	Filter() = default;
	virtual ~Filter() = default;
	virtual String GetName() const { return m_name; }
	virtual bool IsAll() const { return true; }
	virtual FilterType GetType() const { return FilterType::All; }
	virtual Map<int32, ItemIndex> GetFiltered(const Map<int32, ItemIndex>& source) { return source; }
private:
	String m_name = "All";
};

using SongFilter = Filter<SongSelectIndex>;

class LevelFilter : public SongFilter
{
public:
	~LevelFilter() = default;
	LevelFilter(uint16 level) : m_level(level) {}
	virtual Map<int32, SongSelectIndex> GetFiltered(const Map<int32, SongSelectIndex>& source) override;
	virtual String GetName() const override;
	virtual bool IsAll() const override;
	virtual FilterType GetType() const { return FilterType::Level; }


private:
	uint16 m_level;
};

class FolderFilter : public SongFilter
{
public:
	FolderFilter(String folder, MapDatabase* database) : m_folder(folder), m_mapDatabase(database) {}
	~FolderFilter() = default;
	virtual Map<int32, SongSelectIndex> GetFiltered(const Map<int32, SongSelectIndex>& source);
	virtual String GetName() const override;
	virtual bool IsAll() const override;
	virtual FilterType GetType() const { return FilterType::Folder; }


private:
	String m_folder;
	MapDatabase* m_mapDatabase;

};

class CollectionFilter : public SongFilter
{
public:
	CollectionFilter(String collection, MapDatabase* database) : m_collection(collection), m_mapDatabase(database) {}
	~CollectionFilter() = default;

	virtual Map<int32, SongSelectIndex> GetFiltered(const Map<int32, SongSelectIndex>& source);
	virtual String GetName() const override;
	virtual bool IsAll() const override;
	virtual FilterType GetType() const { return FilterType::Collection; }


private:
	String m_collection;
	MapDatabase* m_mapDatabase;

};