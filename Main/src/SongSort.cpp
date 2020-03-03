#include "stdafx.h"
#include "SongSort.hpp"

void TitleSort::SortInplace(Vector<uint32>& vec, const Map<int32, 
		SongSelectIndex>& collection)
{
	std::sort (vec.begin(), vec.end(),
		[&](uint32 a, uint32 b) -> bool
	{
		const SongSelectIndex& song_a = collection.find(a)->second;
		const SongSelectIndex& song_b = collection.find(b)->second;
		return m_dir ^ CompareSongs(song_a, song_b);
	});
}

bool TitleSort::CompareSongs(const SongSelectIndex& song_a,
		const SongSelectIndex& song_b)
{
	return (song_a.GetDifficulties()[0]->settings.title <
			song_b.GetDifficulties()[0]->settings.title);
}

void ScoreSort::SortInplace(Vector<uint32>& vec, const Map<int32, 
		SongSelectIndex>& collection)
{
	m_scoreMap.clear();
	for (uint32 mapIndex : vec)
	{
		const SongSelectIndex& song = collection.find(mapIndex)->second;
		uint32 maxScore = 0;
		for (auto& diff : song.GetDifficulties())
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
			const SongSelectIndex& song_a = collection.find(a)->second;
			const SongSelectIndex& song_b = collection.find(b)->second;
			return m_dir ^ CompareSongs(song_a, song_b);
		}
		return m_dir ^ (score_a < score_b);
	});

	m_scoreMap.clear();
}

void DateSort::SortInplace(Vector<uint32>& vec, const Map<int32, 
		SongSelectIndex>& collection)
{
	m_dateMap.clear();
	for (uint32 mapIndex : vec)
	{
		const SongSelectIndex& song = collection.find(mapIndex)->second;
		uint32 maxDate = 0;
		for (auto& diff : song.GetDifficulties())
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
		const uint32 date_a = m_dateMap.find(a)->second;
		const uint32 date_b = m_dateMap.find(b)->second;

		// For same scores sort by title
		if (date_a == date_b) {
			const SongSelectIndex& song_a = collection.find(a)->second;
			const SongSelectIndex& song_b = collection.find(b)->second;
			return m_dir ^ CompareSongs(song_a, song_b);
		}
		return m_dir ^ (date_a < date_b);
	});

	m_dateMap.clear();
}
