#pragma once

mat4
InitPerspectiveProjection(real32 FOV, real32 AspectRatio, real32 ZN, real32 ZF)
{
    mat4 R = {};
    real32 ZoomX = 1.0f/(tanf(FOV/2.0f)*AspectRatio);
    real32 ZoomY = 1.0f/(tanf(FOV/2.0f));

    R.E[0][0] = ZoomX; R.E[0][1] = 0.0f; R.E[0][2] = 0.0f; R.E[0][3] = 0.0f;
    R.E[1][0] = 0.0f; R.E[1][1] = ZoomY; R.E[1][2] = 0.0f; R.E[1][3] = 0.0f;
    R.E[2][0] = 0.0f; R.E[2][1] = 0.0f; R.E[2][2] = -((ZF + ZN)/(ZF - ZN)); R.E[2][3] = (-2.0f*ZN*ZF)/(ZF - ZN);
    R.E[3][0] = 0.0f; R.E[3][1] = 0.0f; R.E[3][2] = -1.0f; R.E[3][3] = 0.0f;
    
    return R;
}

mat4
InitTranslation(real32 X, real32 Y, real32 Z)
{
    mat4 R = {};

    R.E[0][0] = 1.0f; R.E[0][1] = 0.0f; R.E[0][2] = 0.0f; R.E[0][3] = X;
    R.E[1][0] = 0.0f; R.E[1][1] = 1.0f; R.E[1][2] = 0.0f; R.E[1][3] = Y;
    R.E[2][0] = 0.0f; R.E[2][1] = 0.0f; R.E[2][2] = 1.0f; R.E[2][3] = Z;
    R.E[3][0] = 0.0f; R.E[3][1] = 0.0f; R.E[3][2] = 0.0f; R.E[3][3] = 1.0f;

    return R;
}

mat4
InitRotation(real32 X, real32 Y, real32 Z)
{
    mat4 R = {};

    mat4 XRot = {};
    XRot.E[0][0] = 1.0f; XRot.E[0][1] = 0.0f;     XRot.E[0][2] = 0.0f;    XRot.E[0][3] = 0.0f;
    XRot.E[1][0] = 0.0f; XRot.E[1][1] = cosf(X);  XRot.E[1][2] = sinf(X); XRot.E[1][3] = 0.0f;
    XRot.E[2][0] = 0.0f; XRot.E[2][1] = -sinf(X); XRot.E[2][2] = cosf(X); XRot.E[2][3] = 0.0f;
    XRot.E[3][0] = 0.0f; XRot.E[3][1] = 0.0f;     XRot.E[3][2] = 0.0f;    XRot.E[3][3] = 1.0f;

    mat4 YRot = {};
    YRot.E[0][0] = cosf(Y);  YRot.E[0][1] = 0.0f; YRot.E[0][2] = sinf(Y); YRot.E[0][3] = 0.0f;
    YRot.E[1][0] = 0.0f;     YRot.E[1][1] = 1.0f; YRot.E[1][2] = 0.0f;    YRot.E[1][3] = 0.0f;
    YRot.E[2][0] = -sinf(Y); YRot.E[2][1] = 0.0f; YRot.E[2][2] = cosf(Y); YRot.E[2][3] = 0.0f;
    YRot.E[3][0] = 0.0f;     YRot.E[3][1] = 0.0f; YRot.E[3][2] = 0.0f;    YRot.E[3][3] = 1.0f;

    mat4 ZRot = {};
    ZRot.E[0][0] = cosf(Z);  ZRot.E[0][1] = sinf(Z); ZRot.E[0][2] = 0.0f; ZRot.E[0][3] = 0.0f;
    ZRot.E[1][0] = -sinf(Z); ZRot.E[1][1] = cosf(Z); ZRot.E[1][2] = 0.0f; ZRot.E[1][3] = 0.0f;
    ZRot.E[2][0] = 0.0f;     ZRot.E[2][1] = 0.0f;    ZRot.E[2][2] = 1.0f; ZRot.E[2][3] = 0.0f;
    ZRot.E[3][0] = 0.0f;     ZRot.E[3][1] = 0.0f;    ZRot.E[3][2] = 0.0f; ZRot.E[3][3] = 1.0f;

    R = (ZRot*YRot)*XRot;
    return R;
}

v4
PerspectiveDivide(v4 Vec)
{
    v4 Result = {};
    Result.X = Vec.X/Vec.W;
    Result.Y = Vec.Y/Vec.W;
    Result.Z = Vec.Z/Vec.W;
    Result.W = Vec.W;
    return Result;
}

mat4
CreateLookAt(v4 CameraPos, v4 CameraTarget, v4 CameraUp)
{
    v4 CameraDir = Normalize(CameraPos - CameraTarget);
    v4 CameraRight = Normalize(CrossProduct(CameraUp, CameraDir));

    mat4 CameraPosM = {};
    CameraPosM.E[0][0] = 1.0f; CameraPosM.E[0][1] = 0.0f; CameraPosM.E[0][2] = 0.0f; CameraPosM.E[0][3] = -CameraPos.X;
    CameraPosM.E[1][0] = 0.0f; CameraPosM.E[1][1] = 1.0f; CameraPosM.E[1][2] = 0.0f; CameraPosM.E[1][3] = -CameraPos.Y;
    CameraPosM.E[2][0] = 0.0f; CameraPosM.E[2][1] = 0.0f; CameraPosM.E[2][2] = 1.0f; CameraPosM.E[2][3] = -CameraPos.Z;
    CameraPosM.E[3][0] = 0.0f; CameraPosM.E[3][1] = 0.0f; CameraPosM.E[3][2] = 0.0f; CameraPosM.E[3][3] = 1.0f;

    mat4 CameraDirM = {};
    CameraDirM.E[0][0] = CameraRight.X; CameraDirM.E[0][1] = CameraRight.Y; CameraDirM.E[0][2] = CameraRight.Z; CameraDirM.E[0][3] = 0.0f;
    CameraDirM.E[1][0] = CameraUp.X; CameraDirM.E[1][1] = CameraUp.Y; CameraDirM.E[1][2] = CameraUp.Z; CameraDirM.E[1][3] = 0.0f;
    CameraDirM.E[2][0] = CameraDir.X; CameraDirM.E[2][1] = CameraDir.Y; CameraDirM.E[2][2] = CameraDir.Z; CameraDirM.E[2][3] = 0.0f;
    CameraDirM.E[3][0] = 0.0f; CameraDirM.E[3][1] = 0.0f; CameraDirM.E[3][2] = 0.0f; CameraDirM.E[3][3] = 1.0f;
    mat4 Result = CameraDirM*CameraPosM;
    return Result;
}
