#include "AnimationRuntime.h"

#include <cmath>
#include <map>

namespace
{
    MATRIX MatrixMultiply(const MATRIX& a, const MATRIX& b)
    {
        MATRIX result;

        for (int r = 0; r < 4; ++r)
        {
            for (int c = 0; c < 4; ++c)
            {
                result(r, c) =
                    a(r, 0) * b(0, c) +
                    a(r, 1) * b(1, c) +
                    a(r, 2) * b(2, c) +
                    a(r, 3) * b(3, c);
            }
        }

        return result;
    }

    MATRIX MatrixTranslation(float x, float y, float z)
    {
        MATRIX result;
        result._41 = x;
        result._42 = y;
        result._43 = z;
        return result;
    }

    MATRIX MatrixScaling(float x, float y, float z)
    {
        MATRIX result;
        result._11 = x;
        result._22 = y;
        result._33 = z;
        return result;
    }

    VECTOR3 Vector3Lerp(const VECTOR3& a, const VECTOR3& b, float t)
    {
        return VECTOR3(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t);
    }

    VECTOR4 QuaternionNormalize(const VECTOR4& q)
    {
        const float len = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        if (len <= 0.000001f)
        {
            return VECTOR4(0.0f, 0.0f, 0.0f, 1.0f);
        }

        return VECTOR4(q.x / len, q.y / len, q.z / len, q.w / len);
    }

    VECTOR4 QuaternionNlerp(VECTOR4 a, VECTOR4 b, float t)
    {
        float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        if (dot < 0.0f)
        {
            b.x = -b.x;
            b.y = -b.y;
            b.z = -b.z;
            b.w = -b.w;
        }

        VECTOR4 result(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t);

        return QuaternionNormalize(result);
    }

    MATRIX MatrixRotationQuaternion(const VECTOR4& qv)
    {
        const VECTOR4 q = QuaternionNormalize(qv);

        const float xx = q.x * q.x;
        const float yy = q.y * q.y;
        const float zz = q.z * q.z;
        const float xy = q.x * q.y;
        const float xz = q.x * q.z;
        const float yz = q.y * q.z;
        const float wx = q.w * q.x;
        const float wy = q.w * q.y;
        const float wz = q.w * q.z;

        MATRIX m;
        m._11 = 1.0f - 2.0f * (yy + zz);
        m._12 = 2.0f * (xy + wz);
        m._13 = 2.0f * (xz - wy);
        m._14 = 0.0f;

        m._21 = 2.0f * (xy - wz);
        m._22 = 1.0f - 2.0f * (xx + zz);
        m._23 = 2.0f * (yz + wx);
        m._24 = 0.0f;

        m._31 = 2.0f * (xz + wy);
        m._32 = 2.0f * (yz - wx);
        m._33 = 1.0f - 2.0f * (xx + yy);
        m._34 = 0.0f;

        m._41 = 0.0f;
        m._42 = 0.0f;
        m._43 = 0.0f;
        m._44 = 1.0f;

        return m;
    }

    MATRIX ComposeTransform(const VECTOR3& translation, const VECTOR4& rotation, const VECTOR3& scale)
    {
        const MATRIX s = MatrixScaling(scale.x, scale.y, scale.z);
        const MATRIX r = MatrixRotationQuaternion(rotation);
        const MATRIX t = MatrixTranslation(translation.x, translation.y, translation.z);
        return MatrixMultiply(MatrixMultiply(s, r), t);
    }

    const AnimationClip* FindClip(LPModelData modelData, const std::string& clipName)
    {
        if (modelData == NULL || modelData->Animations.empty())
        {
            return NULL;
        }

        for (std::size_t i = 0; i < modelData->Animations.size(); ++i)
        {
            if (modelData->Animations[i] != NULL && modelData->Animations[i]->Name == clipName)
            {
                return modelData->Animations[i];
            }
        }

        return modelData->Animations[0];
    }

    float NormalizeTime(const AnimationClip* clip, float timeSeconds)
    {
        if (clip == NULL || clip->duration <= 0.0f)
        {
            return 0.0f;
        }

        float t = std::fmod(timeSeconds, clip->duration);
        if (t < 0.0f)
        {
            t += clip->duration;
        }

        return t;
    }

    bool SampleChannel(
        const AnimationClip* clip,
        const std::vector<LPAnimationKeyFrame>& keyFrames,
        float timeSeconds,
        VECTOR3& outTranslation,
        VECTOR4& outRotation,
        VECTOR3& outScale)
    {
        if (keyFrames.empty())
        {
            return false;
        }

        if (keyFrames.size() == 1)
        {
            outTranslation = keyFrames[0]->Translation;
            outRotation = keyFrames[0]->Rotation;
            outScale = keyFrames[0]->Scale;
            return true;
        }

        const float time = NormalizeTime(clip, timeSeconds);

        for (std::size_t i = 0; i + 1 < keyFrames.size(); ++i)
        {
            const LPAnimationKeyFrame a = keyFrames[i];
            const LPAnimationKeyFrame b = keyFrames[i + 1];

            if (time >= a->Time && time <= b->Time)
            {
                float factor = 0.0f;
                const float span = b->Time - a->Time;
                if (span > 0.000001f)
                {
                    factor = (time - a->Time) / span;
                }

                outTranslation = Vector3Lerp(a->Translation, b->Translation, factor);
                outRotation = QuaternionNlerp(a->Rotation, b->Rotation, factor);
                outScale = Vector3Lerp(a->Scale, b->Scale, factor);
                return true;
            }
        }

        const LPAnimationKeyFrame last = keyFrames[keyFrames.size() - 1];
        const LPAnimationKeyFrame first = keyFrames[0];

        float wrapSpan = clip->duration - last->Time + first->Time;
        float factor = 0.0f;
        if (wrapSpan > 0.000001f)
        {
            float dt = (time >= last->Time) ? (time - last->Time) : (time + clip->duration - last->Time);
            factor = dt / wrapSpan;
        }

        outTranslation = Vector3Lerp(last->Translation, first->Translation, factor);
        outRotation = QuaternionNlerp(last->Rotation, first->Rotation, factor);
        outScale = Vector3Lerp(last->Scale, first->Scale, factor);
        return true;
    }

    MATRIX GetLocalMatrixForBone(LPModelData modelData, const AnimationClip* clip, int boneIndex, float timeSeconds)
    {
        if (modelData == NULL || boneIndex < 0 || boneIndex >= static_cast<int>(modelData->Bones.size()))
        {
            return MATRIX();
        }

        const LPBone bone = modelData->Bones[boneIndex];
        if (bone == NULL || clip == NULL)
        {
            return (bone != NULL) ? bone->LocalBindPose : MATRIX();
        }

        std::map<std::string, std::vector<LPAnimationKeyFrame>>::const_iterator it = clip->boneKeyFrames.find(bone->Name);
        if (it == clip->boneKeyFrames.end())
        {
            return bone->LocalBindPose;
        }

        VECTOR3 translation;
        VECTOR4 rotation;
        VECTOR3 scale;
        if (!SampleChannel(clip, it->second, timeSeconds, translation, rotation, scale))
        {
            return bone->LocalBindPose;
        }

        return ComposeTransform(translation, rotation, scale);
    }

    void EvaluateGlobalRecursive(
        LPModelData modelData,
        const AnimationClip* clip,
        int boneIndex,
        float timeSeconds,
        std::vector<MATRIX>& globalTransforms,
        std::vector<char>& evaluated)
    {
        if (evaluated[boneIndex])
        {
            return;
        }

        const LPBone bone = modelData->Bones[boneIndex];
        MATRIX local = GetLocalMatrixForBone(modelData, clip, boneIndex, timeSeconds);

        if (bone != NULL && bone->ParentBoneIndex >= 0 && bone->ParentBoneIndex < static_cast<int>(modelData->Bones.size()))
        {
            EvaluateGlobalRecursive(modelData, clip, bone->ParentBoneIndex, timeSeconds, globalTransforms, evaluated);
            globalTransforms[boneIndex] = MatrixMultiply(local, globalTransforms[bone->ParentBoneIndex]);
        }
        else
        {
            globalTransforms[boneIndex] = local;
        }

        evaluated[boneIndex] = 1;
    }

    void BuildPaletteInternal(LPModelData modelData, const AnimationClip* clip, float timeSeconds, std::vector<MATRIX>& outPalette)
    {
        outPalette.clear();

        if (modelData == NULL || modelData->Bones.empty())
        {
            return;
        }

        std::vector<MATRIX> globalTransforms(modelData->Bones.size());
        std::vector<char> evaluated(modelData->Bones.size(), 0);

        for (int i = 0; i < static_cast<int>(modelData->Bones.size()); ++i)
        {
            EvaluateGlobalRecursive(modelData, clip, i, timeSeconds, globalTransforms, evaluated);
        }

        outPalette.resize(modelData->Bones.size());
        for (int i = 0; i < static_cast<int>(modelData->Bones.size()); ++i)
        {
            const LPBone bone = modelData->Bones[i];
            if (bone != NULL)
            {
                outPalette[i] = MatrixMultiply(bone->OffsetMatrix, globalTransforms[i]);
            }
            else
            {
                outPalette[i] = MATRIX();
            }
        }
    }
}

void AnimationRuntime::BuildBindPosePalette(LPModelData modelData, std::vector<MATRIX>& outPalette)
{
    BuildPaletteInternal(modelData, NULL, 0.0f, outPalette);
}

bool AnimationRuntime::BuildAnimationPalette(LPModelData modelData, const std::string& clipName, float timeSeconds, std::vector<MATRIX>& outPalette)
{
    const AnimationClip* clip = FindClip(modelData, clipName);
    if (clip == NULL)
    {
        BuildBindPosePalette(modelData, outPalette);
        return false;
    }

    BuildPaletteInternal(modelData, clip, timeSeconds, outPalette);
    return true;
}