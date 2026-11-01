// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsTypes.cpp

#include "ScenecapsTypes.h"
#include "Math/UnrealMathUtility.h"

float FScenecapsVariantTransition::Update(float DeltaTime)
{
    if (!bIsActive || Duration <= 0.0f)
    {
        bIsActive = false;
        Alpha = (ToVariant.IsNone() ? 0.0f : 1.0f);
        return Alpha;
    }

    ElapsedTime += DeltaTime;
    Alpha = FMath::Clamp(ElapsedTime / Duration, 0.0f, 1.0f);
    Alpha = EaseAlpha(Alpha, EaseType);

    if (Alpha >= 1.0f)
    {
        if (bPingPong)
        {
            // 反向
            Swap(FromVariant, ToVariant);
            ElapsedTime = 0.0f;
            Alpha = 0.0f;
        }
        else if (bLoop)
        {
            ElapsedTime = 0.0f;
            Alpha = 0.0f;
        }
        else
        {
            bIsActive = false;
            Alpha = 1.0f;
        }
    }

    return Alpha;
}

float FScenecapsVariantTransition::EaseAlpha(float InAlpha, EScenecapsInterpolationEase EaseType)
{
    float t = FMath::Clamp(InAlpha, 0.0f, 1.0f);

    switch (EaseType)
    {
    case EScenecapsInterpolationEase::Linear:
        return t;

    case EScenecapsInterpolationEase::QuadIn:
        return t * t;
    case EScenecapsInterpolationEase::QuadOut:
        return 1.0f - (1.0f - t) * (1.0f - t);
    case EScenecapsInterpolationEase::QuadInOut:
        return t < 0.5f ? 2.0f * t * t : 1.0f - FMath::Pow(-2.0f * t + 2.0f, 2) / 2.0f;

    case EScenecapsInterpolationEase::CubicIn:
        return t * t * t;
    case EScenecapsInterpolationEase::CubicOut:
        return 1.0f - FMath::Pow(1.0f - t, 3);
    case EScenecapsInterpolationEase::CubicInOut:
        return t < 0.5f ? 4.0f * t * t * t : 1.0f - FMath::Pow(-2.0f * t + 2.0f, 3) / 2.0f;

    case EScenecapsInterpolationEase::SineIn:
        return 1.0f - FMath::Cos(t * PI / 2.0f);
    case EScenecapsInterpolationEase::SineOut:
        return FMath::Sin(t * PI / 2.0f);
    case EScenecapsInterpolationEase::SineInOut:
        return -(FMath::Cos(PI * t) - 1.0f) / 2.0f;

    case EScenecapsInterpolationEase::ExpoIn:
        return t == 0.0f ? 0.0f : FMath::Pow(2.0f, 10.0f * (t - 1.0f));
    case EScenecapsInterpolationEase::ExpoOut:
        return t == 1.0f ? 1.0f : 1.0f - FMath::Pow(2.0f, -10.0f * t);
    case EScenecapsInterpolationEase::ExpoInOut:
        if (t == 0.0f) return 0.0f;
        if (t == 1.0f) return 1.0f;
        return t < 0.5f
            ? FMath::Pow(2.0f, 20.0f * t - 10.0f) / 2.0f
            : (2.0f - FMath::Pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;

    default:
        return t;
    }
}
