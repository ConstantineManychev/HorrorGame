#include "CameraSystem.h"

USING_NS_CC;
_CSTART

CameraSystem* CameraSystem::getInstance()
{
	static CameraSystem instance;
	return &instance;
}

CameraSystem::CameraSystem()
	: mCamera(nullptr), mTarget(nullptr), mWorldBounds(Rect::ZERO)
	, mFocusPos(Vec2::ZERO), mCurrentMode(CameraMode::FOLLOW_TARGET)
	, mDefaultZ(0.f), mCurrentZoom(1.0f)
{
	mScreenSize = Director::getInstance()->getWinSize();
	setDeadZoneRatios(1.0f / 3.0f, 1.0f / 3.0f);
}

void CameraSystem::init(cocos2d::Camera* camera, const cocos2d::Rect& worldBounds)
{
	mCamera = camera;
	mWorldBounds = worldBounds;
	mScreenSize = Director::getInstance()->getWinSize();

	if (mCamera) 
	{
		mDefaultZ = mCamera->getPositionZ();
		if (mDefaultZ == 0) mDefaultZ = Director::getInstance()->getZEye();

		mCamera->setPosition(Vec2::ZERO);
		mCamera->setPositionZ(mDefaultZ);
	}
	mCurrentMode = CameraMode::FOLLOW_TARGET;
	mCurrentZoom = 1.0f;
}

void CameraSystem::setMode(CameraMode mode) {
	mCurrentMode = mode;
}

void CameraSystem::setTarget(cocos2d::Node* target) {
	mTarget = target;
	if (mTarget) mFocusPos = mTarget->getPosition();
}

void CameraSystem::setDeadZoneRatios(float widthRatio, float heightRatio) {
	mDeadZoneW = mScreenSize.width * widthRatio;
	mDeadZoneH = mScreenSize.height * heightRatio;
}

// --- Scripted Logic ---

void CameraSystem::moveTo(const Vec2& pos, float duration, std::function<void()> callback) {
	setMode(CameraMode::SCRIPTED);
	mScript.isMoving = true;
	mScript.startPos = mFocusPos;
	mScript.targetPos = pos;
	mScript.duration = duration;
	mScript.elapsed = 0.0f;
	mScript.moveCallback = callback;
}

void CameraSystem::zoomTo(float zoomLevel, float duration, std::function<void()> callback) 
{
	mScript.isZooming = true;
	mScript.startZoom = mCurrentZoom;
	mScript.targetZoom = zoomLevel;

	if (duration <= 0.0f) {
		setZoom(zoomLevel);
		if (callback) callback();
		return;
	}
	mScript.duration = duration;
	mScript.elapsed = 0.0f;
	mScript.zoomCallback = callback;
}

// --- Manual Logic ---

void CameraSystem::panCamera(const Vec2& delta) {
	if (mCurrentMode == CameraMode::MANUAL) {
		mFocusPos -= delta;
	}
}

void CameraSystem::setZoom(float zoomLevel) {
	mCurrentZoom = clampf(zoomLevel, 0.1f, 5.0f);
	if (mCamera) {
		mCamera->setPositionZ(mDefaultZ / mCurrentZoom);
	}
}

// --- Update Loop ---

void CameraSystem::update(float dt)
{
	if (!mCamera) return;

	if (mCurrentMode == CameraMode::FOLLOW_TARGET && mTarget)
	{
		Vec2 targetPos = mTarget->getPosition();
		float dx = targetPos.x - mFocusPos.x;
		float dy = targetPos.y - mFocusPos.y;
		float zoneHalfW = mDeadZoneW * 0.5f / mCurrentZoom;
		float zoneHalfH = mDeadZoneH * 0.5f / mCurrentZoom;

		if (dx > zoneHalfW) mFocusPos.x = targetPos.x - zoneHalfW;
		else if (dx < -zoneHalfW) mFocusPos.x = targetPos.x + zoneHalfW;

		if (dy > zoneHalfH) mFocusPos.y = targetPos.y - zoneHalfH;
		else if (dy < -zoneHalfH) mFocusPos.y = targetPos.y + zoneHalfH;
	}
	else if (mCurrentMode == CameraMode::SCRIPTED)
	{
		mScript.elapsed += dt;
		float t = clampf(mScript.elapsed / mScript.duration, 0.f, 1.f);

		float smoothT = -0.5f * (cosf(M_PI * t) - 1.0f);

		if (mScript.isMoving) {
			mFocusPos = mScript.startPos.lerp(mScript.targetPos, smoothT);
		}
		if (mScript.isZooming) {
			float newZoom = mScript.startZoom + (mScript.targetZoom - mScript.startZoom) * smoothT;
			setZoom(newZoom);
		}

		if (t >= 1.0f) {
			if (mScript.isMoving) {
				mScript.isMoving = false;
				if (mScript.moveCallback) mScript.moveCallback();
			}
			if (mScript.isZooming) {
				mScript.isZooming = false;
				if (mScript.zoomCallback) mScript.zoomCallback();
			}
		}
	}

	// 2. Clamping
	float halfScreenW = (mScreenSize.width / mCurrentZoom) * 0.5f;
	float halfScreenH = (mScreenSize.height / mCurrentZoom) * 0.5f;

	float minX = mWorldBounds.getMinX() + halfScreenW;
	float maxX = mWorldBounds.getMaxX() - halfScreenW;
	float minY = mWorldBounds.getMinY() + halfScreenH;
	float maxY = mWorldBounds.getMaxY() - halfScreenH;

	if (minX > maxX) minX = maxX = mWorldBounds.getMidX();
	if (minY > maxY) minY = maxY = mWorldBounds.getMidY();

	// 
	float clampedX = std::max(minX, std::min(mFocusPos.x, maxX));
	float clampedY = std::max(minY, std::min(mFocusPos.y, maxY));

	if (mCurrentMode == CameraMode::MANUAL) {
		mFocusPos = Vec2(clampedX, clampedY);
	}

	mCamera->setPosition(Vec2(clampedX, clampedY));
}

void CameraSystem::reset() {
	mCamera = nullptr;
	mTarget = nullptr;
}

_CEND