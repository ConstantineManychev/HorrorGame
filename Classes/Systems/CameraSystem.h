#ifndef __CAMERA_SYSTEM_H__
#define __CAMERA_SYSTEM_H__

#include "cocos2d.h"
#include "CommonDefines.h"
#include "Basics/ServiceLocator.h"
#include <functional>

_CSTART

enum class CameraMode {
	FOLLOW_TARGET,
	SCRIPTED,
	MANUAL
};

class CameraSystem
{
	friend class ServiceLocator;
	friend class AppDelegate;

public:

	void init(cocos2d::Camera* camera, const cocos2d::Rect& worldBounds);
	void reset();

	void setMode(CameraMode mode);
	CameraMode getMode() const { return mCurrentMode; }

	// -- FOLLOW_TARGET mode --
	void setTarget(cocos2d::Node* target);
	void setDeadZoneRatios(float widthRatio, float heightRatio);

	// -- SCRIPTED mode --
	void moveTo(const cocos2d::Vec2& pos, float duration, std::function<void()> callback = nullptr);
	void zoomTo(float zoomLevel, float duration, std::function<void()> callback = nullptr);

	// -- MANUAL mode --
	void panCamera(const cocos2d::Vec2& delta);
	void setZoom(float zoomLevel);

	void update(float dt);

private:
	CameraSystem();

	cocos2d::Camera* mCamera;
	cocos2d::Node* mTarget;
	cocos2d::Rect mWorldBounds;
	cocos2d::Vec2 mFocusPos;
	cocos2d::Size mScreenSize;
	float mDeadZoneW;
	float mDeadZoneH;

	CameraMode mCurrentMode;
	float mDefaultZ;

	// Interpolation
	struct {
		cocos2d::Vec2 startPos;
		cocos2d::Vec2 targetPos;
		float startZoom;
		float targetZoom;
		float duration;
		float elapsed;
		bool isMoving;
		bool isZooming;
		std::function<void()> moveCallback;
		std::function<void()> zoomCallback;
	} mScript;

	float mCurrentZoom;
};

#define CAM_SYS SL->getService<CameraSystem>()

_CEND

#endif