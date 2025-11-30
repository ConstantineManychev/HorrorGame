#include "GestureRecognizer.h"
#include <cmath>
#include <limits>
#include <set>
#include <algorithm>

USING_NS_CC;
_CSTART

GestureRecognizer* GestureRecognizer::getInstance()
{
	static GestureRecognizer instance;
	return &instance;
}

GestureRecognizer::GestureRecognizer()
{
	mReferenceLetters = {
		{'A', { {0, 0}, {0.5, 1}, {1, 0}, {0.5, 0.5} } },
		{'B', { {0, 0}, {0, 1}, {1, 0.75}, {1, 0.25} } },
		{'C', { {0.5, 0}, {0.5, 1}, {0, 0.5} } },
		{'D', { {0, 0}, {0, 1}, {1, 0.5} } },
		{'E', { {0, 0}, {0, 1}, {1, 1}, {1, 0.5}, {1, 0} } },
		{'F', { {0, 0}, {0, 1}, {1, 1}, {1, 0.5} } },
		{'G', { {0, 0}, {0, 1}, {1, 1}, {1, 0.75}, {0.5, 0.5} } },
		{'H', { {0, 0}, {0, 1}, {1, 1}, {1, 0}, {0.5, 0.5} } },
		{'I', { {0.5, 0}, {0.5, 1} } },
		{'J', { {1, 0}, {0.5, 0}, {0.5, 1}, {0, 1}, {0, 0.5} } },
		{'K', { {0, 0}, {0, 1}, {1, 0.5}, {0, 0.75}, {1, 0.25} } },
		{'L', { {0, 0}, {0, 1}, {1, 1} } },
		{'M', { {0, 0}, {0, 1}, {0.5, 0.5}, {1, 1}, {1, 0} } },
		{'N', { {0, 0}, {0, 1}, {1, 0}, {1, 1}, {0.5, 0.5}, {0.75, 0.25}, {0.25, 0.75} } },
		{'O', { {0.5, 0}, {0, 0.5}, {0.5, 1}, {1, 0.5} } },
		{'P', { {0, 0}, {0, 1}, {1, 1}, {1, 0.5}, {0, 0.5} } },
		{'Q', { {0.5, 0}, {0, 0.5}, {0.5, 1}, {1, 0.5}, {0.5, 0.25} } },
		{'R', { {0, 0}, {0, 1}, {1, 1}, {1, 0.5}, {0, 0.5}, {1, 0} } },
		{'S', { {0.5, 0}, {0, 0.5}, {0.5, 1}, {1, 0.5}, {0.5, 0.25} } },
		{'T', { {0, 1}, {1, 1}, {0.5, 0} } },
		{'U', { {0, 0}, {0, 1}, {1, 1}, {1, 0} } },
		{'V', { {0, 1}, {0.5, 0}, {1, 1} } },
		{'W', { {0, 1}, {0.5, 0}, {1, 1}, {1, 0}, {0.5, 0.5} } },
		{'X', { {0, 0}, {1, 1}, {1, 0}, {0, 1} } },
		{'Y', { {0, 1}, {0.5, 0}, {1, 1}, {0.5, 0.5} } },
		{'Z', { {0, 1}, {1, 1}, {0, 0}, {1, 0} } }
	};
}

char GestureRecognizer::recognize(const std::vector<Vec2>& points)
{
	if (points.empty()) return '?';

	std::vector<Vec2> normalizedInput = normalizePoints(points);

	char bestMatch = '?';
	float minDistance = 0.08f;

	for (const auto& ref : mReferenceLetters)
	{
		float distance = calculateTotalDistance(normalizedInput, ref.second);
		if (distance < minDistance)
		{
			minDistance = distance;
			bestMatch = ref.first;
		}
	}
	return bestMatch;
}

float GestureRecognizer::roundToQuarter(float value)
{
	return std::round(value * 4) / 4.0f;
}

std::vector<Vec2> GestureRecognizer::normalizePoints(const std::vector<Vec2>& aPoints)
{
	std::vector<Vec2> normalizedPoints;
	if (!aPoints.empty())
	{
		float minX = aPoints[0].x, minY = aPoints[0].y;
		float maxX = minX, maxY = minY;

		for (const auto& point : aPoints)
		{
			minX = std::min(minX, point.x);
			minY = std::min(minY, point.y);
			maxX = std::max(maxX, point.x);
			maxY = std::max(maxY, point.y);
		}

		std::set<Vec2> uniqueNormalizedPoints;

		float rangeX = maxX - minX;
		float rangeY = maxY - minY;
		if (rangeX == 0) rangeX = 1.0f;
		if (rangeY == 0) rangeY = 1.0f;

		for (const auto& point : aPoints)
		{
			float normX = (point.x - minX) / rangeX;
			float normY = (point.y - minY) / rangeY;

			normX = roundToQuarter(normX);
			normY = roundToQuarter(normY);

			uniqueNormalizedPoints.emplace(normX, normY);
		}

		normalizedPoints.assign(uniqueNormalizedPoints.begin(), uniqueNormalizedPoints.end());
	}
	return normalizedPoints;
}

float GestureRecognizer::calculateDistance(const Vec2& aPoint1, const Vec2& aPoint2)
{
	return aPoint1.distance(aPoint2);
}

float GestureRecognizer::calculateTotalDistance(const std::vector<Vec2>& aPoints, const std::vector<Vec2>& aLetterPoints)
{
	float totalDistance = 0.0f;
	for (const auto& letterPoint : aLetterPoints)
	{
		float letterPointDistance = std::numeric_limits<float>::max();
		for (const auto& point : aPoints)
		{
			float distance = calculateDistance(point, letterPoint);
			if (distance < letterPointDistance)
			{
				letterPointDistance = distance;
			}
		}
		totalDistance += letterPointDistance;
	}
	return totalDistance / (aLetterPoints.empty() ? 1 : aLetterPoints.size());
}

_CEND