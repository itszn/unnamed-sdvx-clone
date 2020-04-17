#include "stdafx.h"
#include "SongSort.hpp"
#include "Shared/Profiling.hpp"

const SongSelectIndex& getSongFromCollection(uint32 index, const Map<int32,
	SongSelectIndex>& collection)
{
	auto it = collection.find(index);
	if (it == collection.end())
	{
		Logf("Could not find song id %u", Logger::Error, index);
	}
	return it->second;
}

void TitleSort::SortInplace(Vector<uint32>& vec, const Map<int32, 
		SongSelectIndex>& collection)
{
	ProfilerScope $(Utility::Sprintf("Sort by: %s", m_name));
	std::sort (vec.begin(), vec.end(),
		[&](uint32 a, uint32 b) -> bool
	{
		const SongSelectIndex& song_a = getSongFromCollection(a, collection);
		const SongSelectIndex& song_b = getSongFromCollection(b, collection);
		bool res = CompareSongs(song_a, song_b);
		return m_dir ? !res : res;
	});
}

bool TitleSort::CompareSongs(const SongSelectIndex& song_a,
		const SongSelectIndex& song_b)
{
	String a = song_a.GetCharts()[0]->title;
	String b = song_b.GetCharts()[0]->title;
	a.ToUpper();
	b.ToUpper();
	int strres = a.compare(b);
	if (strres == 0)
		return song_a.id < song_b.id;
	return strres < 0;
}

void ScoreSort::SortInplace(Vector<uint32>& vec, const Map<int32, 
		SongSelectIndex>& collection)
{
	ProfilerScope $(Utility::Sprintf("Sort by: %s", m_name));
	m_scoreMap.clear();
	for (uint32 mapIndex : vec)
	{
		const SongSelectIndex& song = getSongFromCollection(mapIndex, collection);
		uint32 maxScore = 0;
		for (auto& diff : song.GetCharts())
		{
			for (auto& score : diff->scores)
			{
				if (score->score < maxScore)
					continue;
				maxScore = score->score;
			}
		}
		m_scoreMap[mapIndex] = maxScore;
	}

	std::sort (vec.begin(), vec.end(),
		[&](uint32 a, uint32 b) -> bool
	{
		const uint32 score_a = m_scoreMap.find(a)->second;
		const uint32 score_b = m_scoreMap.find(b)->second;

		// For same scores sort by title
		if (score_a == score_b) {
			const SongSelectIndex& song_a = getSongFromCollection(a, collection);
			const SongSelectIndex& song_b = getSongFromCollection(b, collection);
			bool res = CompareSongs(song_a, song_b);
			return res;
		}
		bool res = score_a < score_b;
		return m_dir ? !res : res;
	});

	m_scoreMap.clear();
}

void DateSort::SortInplace(Vector<uint32>& vec, const Map<int32, 
		SongSelectIndex>& collection)
{
	ProfilerScope $(Utility::Sprintf("Sort by: %s", m_name));
	m_dateMap.clear();
	for (uint32 mapIndex : vec)
	{
		const SongSelectIndex& song = getSongFromCollection(mapIndex, collection);
		uint64 maxDate = 0;
		for (auto& diff : song.GetCharts())
		{
			if (diff->lwt < maxDate)
				continue;
			maxDate = diff->lwt;
		}
		m_dateMap[mapIndex] = maxDate;
	}

	std::sort (vec.begin(), vec.end(),
		[&](uint32 a, uint32 b) -> bool
	{
		const uint64 date_a = m_dateMap.find(a)->second;
		const uint64 date_b = m_dateMap.find(b)->second;

		// For same scores sort by title
		if (date_a == date_b) {
			const SongSelectIndex& song_a = getSongFromCollection(a, collection);
			const SongSelectIndex& song_b = getSongFromCollection(b, collection);
			bool res = CompareSongs(song_a, song_b);
			return res;
		}
		bool res = date_a < date_b;
		return m_dir ? !res : res;
	});

	m_dateMap.clear();
}
