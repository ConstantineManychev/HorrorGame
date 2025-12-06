#include "CutsceneManager.h"
#include "Helpers/JsonHelper.h"
#include "Managers/GameDirector.h"
#include "Managers/ViewManager.h"
#include "Managers/AudioManager.h"
#include "Systems/CameraSystem.h"
#include "ScenesManager.h"

USING_NS_CC;
_CSTART

CutsceneManager* CutsceneManager::getInstance()
{
	static CutsceneManager instance;
	return &instance;
}

CutsceneManager::CutsceneManager()
	: mIsPlaying(false)
{
}

CutsceneManager::~CutsceneManager()
{
}

void CutsceneManager::playCutscene(const std::string& aConfigPath, std::function<void()> onComplete)
{
	if (mIsPlaying)
	{
		stopCutscene();
	}

	mIsPlaying = true;
	mOnCompleteCallback = onComplete;

	parseCutsceneConfig(aConfigPath);
	processNextStep();
}

void CutsceneManager::stopCutscene()
{
	mIsPlaying = false;
	while (!mStepsQueue.empty()) mStepsQueue.pop();

	CAM_SYS->setMode(CameraMode::FOLLOW_TARGET);

	if (auto player = GD->getCurrentPlayer())
	{
		CAM_SYS->setTarget(player);
		// player->setInputEnabled(true);
	}
}

void CutsceneManager::finishCutscene()
{
	stopCutscene();
	if (mOnCompleteCallback)
	{
		mOnCompleteCallback();
		mOnCompleteCallback = nullptr;
	}
}

void CutsceneManager::parseCutsceneConfig(const std::string& aConfigPath)
{
	Value val = JsonHelper::getValueFromJson(aConfigPath);
	if (val.getType() == Value::Type::MAP)
	{
		ValueMap map = val.asValueMap();
		if (map.count("sequence") && map.at("sequence").getType() == Value::Type::VECTOR)
		{
			const auto& seq = map.at("sequence").asValueVector();
			for (const auto& stepVal : seq)
			{
				if (stepVal.getType() == Value::Type::MAP)
				{
					ValueMap stepMap = stepVal.asValueMap();
					CutsceneStep step;
					if (stepMap.count("action")) // "action" is the type key in your json
					{
						step.type = stepMap.at("action").asString();
						step.params = stepMap;
						mStepsQueue.push(step);
					}
				}
			}
		}
	}
}

void CutsceneManager::processNextStep()
{
	if (!mIsPlaying) return;

	if (mStepsQueue.empty())
	{
		finishCutscene();
		return;
	}

	CutsceneStep currentStep = mStepsQueue.front();
	mStepsQueue.pop();

	std::string type = currentStep.type;
	const auto& params = currentStep.params;

	if (type == "wait") handleWait(params);
	else if (type == "camera_move") handleCameraMove(params);
	else if (type == "camera_zoom") handleCameraZoom(params);
	else if (type == "node_action" || type == "fade_in" || type == "fade_out") handleNodeAction(params); // Support inline types
	else if (type == "change_view") handleChangeView(params);
	else if (type == "play_sound") handlePlaySound(params);
	else
	{
		CCLOG("CutsceneManager: Unknown action type '%s', skipping.", type.c_str());
		processNextStep();
	}
}

// --- Handlers ---

void CutsceneManager::handleWait(const ValueMap& params)
{
	float duration = params.count("duration") ? params.at("duration").asFloat() : 0.0f;

	auto scene = Director::getInstance()->getRunningScene();
	if (scene)
	{
		scene->runAction(Sequence::create(
			DelayTime::create(duration),
			CallFunc::create([this]() { this->processNextStep(); }),
			nullptr
		));
	}
	else
	{
		processNextStep();
	}
}

void CutsceneManager::handleCameraMove(const ValueMap& params)
{
	float duration = params.count("duration") ? params.at("duration").asFloat() : 1.0f;
	Vec2 targetPos = Vec2::ZERO;

	if (params.count("target_node"))
	{
		Node* target = findNodeInScene(params.at("target_node").asString());
		if (target) targetPos = target->getPosition();
	}
	else if (params.count("pos"))
	{
		targetPos = JsonHelper::toVec2(params.at("pos"));
	}

	CAM_SYS->moveTo(targetPos, duration, [this]() {
		this->processNextStep();
	});
}

void CutsceneManager::handleCameraZoom(const ValueMap& params)
{
	float duration = params.count("duration") ? params.at("duration").asFloat() : 1.0f;
	float zoom = params.count("zoom") ? params.at("zoom").asFloat() : 1.0f;

	CAM_SYS->zoomTo(zoom, duration, [this]() {
		this->processNextStep();
	});
}

void CutsceneManager::handleNodeAction(const ValueMap& params)
{
	std::string nodeName = params.count("target_node") ? params.at("target_node").asString() : "";
	std::string actionType = params.count("action") ? params.at("action").asString() : "";
	float duration = params.count("duration") ? params.at("duration").asFloat() : 0.0f;

	// Check if the action itself was the type (short syntax in json)
	if (actionType.empty() && params.count("type")) actionType = params.at("type").asString();

	Node* target = findNodeInScene(nodeName);
	if (!target)
	{
		CCLOG("CutsceneManager: Node '%s' not found.", nodeName.c_str());
		processNextStep();
		return;
	}

	FiniteTimeAction* action = nullptr;

	if (actionType == "fade_in")
	{
		action = FadeIn::create(duration);
	}
	else if (actionType == "fade_out")
	{
		action = FadeOut::create(duration);
	}
	else if (actionType == "move_to")
	{
		Vec2 pos = JsonHelper::toVec2(params.at("pos"));
		action = MoveTo::create(duration, pos);
	}

	if (action)
	{
		bool waitForFinish = params.count("wait_for_finish") ? params.at("wait_for_finish").asBool() : true;

		if (waitForFinish)
		{
			target->runAction(Sequence::create(action, CallFunc::create([this]() {
				this->processNextStep();
			}), nullptr));
		}
		else
		{
			target->runAction(action);
			processNextStep();
		}
	}
	else
	{
		processNextStep();
	}
}

void CutsceneManager::handleChangeView(const ValueMap& params)
{
	std::string viewID = params.count("view_id") ? params.at("view_id").asString() : "";
	if (!viewID.empty())
	{
		VM->changeView(viewID);
	}
	// Give it a frame to load
	Director::getInstance()->getScheduler()->performFunctionInCocosThread([this]() {
		this->processNextStep();
	});
}

void CutsceneManager::handlePlaySound(const ValueMap& params)
{
	if (params.count("file"))
	{
		std::string file = params.at("file").asString();
		bool loop = params.count("loop") ? params.at("loop").asBool() : false;

		if (params.count("type") && params.at("type").asString() == "music")
			AM->playMusic(file, loop);
		else
			AM->playSFX(file, loop);
	}
	processNextStep();
}

// --- Helpers ---

cocos2d::Node* CutsceneManager::findNodeInScene(const std::string& name)
{
	auto scene = SM->getCurrentScene();
	if (!scene) return nullptr;
	return findNodeRecursive(scene, name);
}

cocos2d::Node* CutsceneManager::findNodeRecursive(cocos2d::Node* root, const std::string& name)
{
	if (root->getName() == name) return root;

	for (auto child : root->getChildren())
	{
		Node* res = findNodeRecursive(child, name);
		if (res) return res;
	}
	return nullptr;
}

_CEND