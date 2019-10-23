#pragma once
#include "ApplicationTickable.hpp"
#include "AsyncLoadable.hpp"

/*
	Loading and transition screen
*/
class ReplayScreen : public IApplicationTickable
{
public:
	virtual ~ReplayScreen() = default;
	static ReplayScreen* Create(IAsyncLoadableApplicationTickable* next, bool noCancel = false);
	static ReplayScreen* Create(class Game* next);

	Delegate<IAsyncLoadableApplicationTickable*> OnLoadingComplete;
};