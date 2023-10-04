#pragma once 
union v4 
{
    struct
    {
        real32 X;
        real32 Y;
        real32 Z;
        real32 W;
    };
    real32 E[4];
};

struct mat4
{
    real32 E[4][4];
};

v4
operator*(mat4 M, v4 V)
{
    v4 Result;
    Result.X = V.X*M.E[0][0] + V.Y*M.E[0][1] + V.Z*M.E[0][2] + V.W*M.E[0][3];
    Result.Y = V.X*M.E[1][0] + V.Y*M.E[1][1] + V.Z*M.E[1][2] + V.W*M.E[1][3];
    Result.Z = V.X*M.E[2][0] + V.Y*M.E[2][1] + V.Z*M.E[2][2] + V.W*M.E[2][3];
    Result.W = V.X*M.E[3][0] + V.Y*M.E[3][1] + V.Z*M.E[3][2] + V.W*M.E[3][3];
    return Result;
}

mat4
operator*(mat4 M1, mat4 M2)
{
    mat4 R = {};
    
    for(int32 J = 0;
        J < 4;
        J++)
    {
        for(int32 I = 0;
            I < 4;
            I++)
        {
            R.E[J][I] = M1.E[J][0]*M2.E[0][I] + 
                        M1.E[J][1]*M2.E[1][I] + 
                        M1.E[J][2]*M2.E[2][I] + 
                        M1.E[J][3]*M2.E[3][I];
        }
    }
    return R;
}

v4 
operator-(v4 V1, v4 V2)
{
    v4 Result = {};
    Result.X = V1.X - V2.X;
    Result.Y = V1.Y - V2.Y;
    Result.Z = V1.Z - V2.Z;
    Result.W = V1.W - V2.W;
    return Result;
}

v4 
operator+(v4 V1, v4 V2)
{
    v4 Result = {};
    Result.X = V1.X + V2.X;
    Result.Y = V1.Y + V2.Y;
    Result.Z = V1.Z + V2.Z;
    Result.W = V1.W + V2.W;
    return Result;
}

v4 &
operator+=(v4& V1, v4 V2)
{
    V1 = V1 + V2;
    return V1;
}

v4 &
operator-=(v4& V1, v4 V2)
{
    V1 = V1 - V2;
    return V1;
}

v4 
operator*(v4 V, real32 K)
{
    v4 Result = {};
    Result.X = V.X*K;
    Result.Y = V.Y*K;
    Result.Z = V.Z*K;
    Result.W = V.W*K;
    return Result;
}

int32 
Power(int32 Base, int32 Power)
{
    int32 Result = 1;
    for(int32 I = 0;
        I < Power;
        I++)
    {
        Result *= Base;
    }
    return Result;
}

v4 
Lerp(v4 V1, v4 V2, real32 LerpFactor)
{
    v4 Result = {};
    Result = (V2 - V1)*LerpFactor + V1;
    return Result;
}

v4
CrossProduct(v4 V1, v4 V2)
{
    v4 Result = {};
    Result.X = V1.Y*V2.Z - V1.Z*V2.Y;
    Result.Y = V1.Z*V2.X - V1.X*V2.Z;
    Result.Z = V1.X*V2.Y - V1.Y*V2.X;
    Result.W = 0.0f;
    return Result;
}

real32 
Squared(real32 Value)
{
    return Value*Value;
}

real32 
LengthV(v4 Vec)
{
    real32 Result = 0.0f;
    for(int32 I = 0;
        I < 3;
        I++)
    {
        Result += Squared(Vec.E[I]);
    }
    return sqrt(Result);
}

v4
Normalize(v4 Vec)
{
    v4 Result = {};
    real32 Length = LengthV(Vec);
    for(int32 I = 0;
        I < 3;
        I++)
    {
        Result.E[I] = Vec.E[I]/Length;
    }
    return Result;
}

real32 
DotProduct(v4 V1, v4 V2)
{
    real32 Result = 0.0f;
    for(int32 I = 0;
        I < 3;
        I++)
    {
        Result += V1.E[I]*V2.E[I];
    }
    return Result;
}

real32
Saturate(real32 Value)
{
    real32 Result = Value;
    if(Value < 0.0f)
    {
        Result = 0.0f;  
    }
    if(Value > 1.0f)
    {
        Result = 1.0f;
    }
    return Result;
}


