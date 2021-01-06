#pragma once
#include "ApplicationTickable.hpp"
#include "AsyncLoadable.hpp"
#include "json.hpp"

class ChallengeResultScreen : public IAsyncLoadableApplicationTickable
{
public:
	virtual ~ChallengeResultScreen() = default;
	static ChallengeResultScreen* Create(class ChallengeManager*);
};
