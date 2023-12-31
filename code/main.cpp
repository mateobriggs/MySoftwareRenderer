#include <Windows.h>
#include <stdint.h> 
#include <math.h> 
#include <stdio.h>

#define uint8  uint8_t
#define uint16 uint16_t
#define uint32 uint32_t
#define uint64 uint64_t

#define int8  int8_t
#define int16 int16_t
#define int32 int32_t
#define int64 int64_t

#define bool32 int32
#define real32 float
#define real64 double

static bool GlobalRunning = false;

struct read_file_result
{
    void *Contents;
    uint32 ContentsSize;
};

struct texture
{
    uint32 *Bitmap;
    uint32 Width;
    uint32 Height;
};

static texture GlobalTexture;

#include "math.h"

struct vertex
{
    v4 Pos;
    v4 TexCoords;
    v4 Normal;
};

#include "transform.h"

struct mesh
{
    vertex *Vertices;
    uint32 VertexAmount;
};

#include "OBJ.cpp"

struct edge
{
    int32 YMin;
    int32 YMax;
    real32 X;
    real32 XStep;

    real32 XTexCoord;
    real32 YTexCoord;

    real32 XTexCoordStep;
    real32 YTexCoordStep;

    real32 OneOverZ;
    real32 OneOverZStep;

    real32 Depth;
    real32 DepthStep;

    real32 LightAmount;
    real32 LightAmountStep;
};

struct gradients
{
    real32 XTexCoords[3];
    real32 YTexCoords[3];
    real32 OneOverZ[3];
    real32 Depth[3];
    real32 LightAmount[3];

    real32 XTexCoordsXStep;
    real32 XTexCoordsYStep;

    real32 YTexCoordsXStep;
    real32 YTexCoordsYStep;

    real32 OneOverZXStep;
    real32 OneOverZYStep;

    real32 DepthXStep;
    real32 DepthYStep;

    real32 LightAmountXStep;
    real32 LightAmountYStep;
};

struct back_buffer
{
    BITMAPINFO Info;
    void* Memory;
    int32 Width;
    int32 Height;
    int32 Pitch;
    int32 BytesPerPixel;
};

static back_buffer GlobalBackBuffer;

struct z_buffer
{
    real32 *Memory;
    int32 Width;
    int32 Height;
};

static z_buffer ZBuffer; 

void 
ClearZBuffer()
{
    for(int32 Y = 0;
        Y < ZBuffer.Height;
        Y++)
    {
        for(int32 X = 0;
            X < ZBuffer.Width;
            X++)
        {
            ZBuffer.Memory[Y*ZBuffer.Width + X] = 1000.0f;
        }
    }
}

void
ClearVertexArray(vertex *Array, int32 *ArraySize)
{
    for(int32 I = 0;
        I < (*ArraySize);
        I++)
    {
        Array[I] = {};
    }
    *ArraySize = 0;
}

vertex
Lerp(vertex V1, vertex V2, real32 LerpFactor)
{
    vertex Result = {};
    Result.Pos = Lerp(V1.Pos, V2.Pos, LerpFactor);
    Result.TexCoords = Lerp(V1.TexCoords, V2.TexCoords, LerpFactor);
    Result.Normal = Lerp(V1.Normal, V2.Normal, LerpFactor);
    return Result;
}

void
ClipVertexComponent(vertex *Vertices, int32 *VertAmount, 
                    vertex *AuxVertices, int32 *VertAuxAmount, 
                    int32 ComponentIndex, real32 ComponentFactor)
{   
    vertex PreviousVertex = Vertices[(*VertAmount) - 1]; 
    real32 PreviousComponent = PreviousVertex.Pos.E[ComponentIndex]*ComponentFactor;
    bool32 IsPreviousInside = PreviousVertex.Pos.W >= PreviousComponent;

    for(int32 I = 0; 
        I < (*VertAmount);
        I++)
    {
        vertex CurrentVertex = Vertices[I]; 
        real32 CurrentComponent = CurrentVertex.Pos.E[ComponentIndex]*ComponentFactor;
        bool32 IsCurrentInside = CurrentVertex.Pos.W >= CurrentComponent;
        if(IsPreviousInside ^ IsCurrentInside)
        {
            real32 LerpFactor = (PreviousVertex.Pos.W - PreviousComponent)/
                                ((PreviousVertex.Pos.W - PreviousComponent) - 
                                 (CurrentVertex.Pos.W - CurrentComponent));
            vertex NewVertex = Lerp(PreviousVertex, CurrentVertex, LerpFactor);
            AuxVertices[(*VertAuxAmount)] = NewVertex;
            (*VertAuxAmount)++;
        }
        if(IsCurrentInside)
        {
            AuxVertices[(*VertAuxAmount)] = CurrentVertex;
            (*VertAuxAmount)++;
        }
        PreviousVertex = CurrentVertex;
        PreviousComponent = CurrentComponent;
        IsPreviousInside = IsCurrentInside;
    }

}

bool32 
ClipVertexAxis(vertex *Vertices, int32 *VertAmount, vertex *AuxVertices, 
                int32 *VertAuxAmount, int32 ComponentIndex)
{
    bool32 Result = false;
    ClipVertexComponent(Vertices, VertAmount, AuxVertices, VertAuxAmount, 
                            ComponentIndex, 1.0f);
    ClearVertexArray(Vertices, VertAmount);
    if(!(*VertAuxAmount))
    {
        return Result;
    }

    ClipVertexComponent(AuxVertices, VertAuxAmount, Vertices, VertAmount, 
                            ComponentIndex, -1.0f);
    ClearVertexArray(AuxVertices, VertAuxAmount);
    Result = *VertAmount;
    return Result;
}




real32 
DInterpolant(real32 Interpolants[3], real32 Pos0, real32 Pos1, real32 Pos2)
{
    real32 Result = (Interpolants[1] - Interpolants[2])*(Pos0 - Pos2) -
                    (Interpolants[0] - Interpolants[2])*(Pos1 - Pos2);
    return Result;
}

gradients
CreateGradients(vertex Ver0, vertex Ver1, vertex Ver2)
{
    gradients Result = {};

    Result.Depth[0] = Ver0.Pos.Z;
    Result.Depth[1] = Ver1.Pos.Z;
    Result.Depth[2] = Ver2.Pos.Z;

    Result.OneOverZ[0] = 1.0f/Ver0.Pos.W;
    Result.OneOverZ[1] = 1.0f/Ver1.Pos.W;
    Result.OneOverZ[2] = 1.0f/Ver2.Pos.W;

    Result.XTexCoords[0] = Ver0.TexCoords.X*Result.OneOverZ[0];
    Result.XTexCoords[1] = Ver1.TexCoords.X*Result.OneOverZ[1];
    Result.XTexCoords[2] = Ver2.TexCoords.X*Result.OneOverZ[2];


    Result.YTexCoords[0] = Ver0.TexCoords.Y*Result.OneOverZ[0];
    Result.YTexCoords[1] = Ver1.TexCoords.Y*Result.OneOverZ[1];
    Result.YTexCoords[2] = Ver2.TexCoords.Y*Result.OneOverZ[2];

    v4 LightDirection = {0.0f, 0.0f, -1.0f, 0.0f};
    Result.LightAmount[0] = Saturate(DotProduct(Ver0.Normal, LightDirection))*0.9f + 0.1f;
    Result.LightAmount[1] = Saturate(DotProduct(Ver1.Normal, LightDirection))*0.9f + 0.1f;
    Result.LightAmount[2] = Saturate(DotProduct(Ver2.Normal, LightDirection))*0.9f + 0.1f;

    real32 OneOverDx = 1.0f/((Ver1.Pos.X - Ver2.Pos.X)*(Ver0.Pos.Y - Ver2.Pos.Y) - 
                        (Ver0.Pos.X - Ver2.Pos.X)*(Ver1.Pos.Y - Ver2.Pos.Y));
    real32 OneOverDy = -OneOverDx;

    real32 DXTexCoordX = DInterpolant(Result.XTexCoords, Ver0.Pos.Y, Ver1.Pos.Y, Ver2.Pos.Y);
    real32 DXTexCoordY = DInterpolant(Result.XTexCoords, Ver0.Pos.X, Ver1.Pos.X, Ver2.Pos.X);
    Result.XTexCoordsXStep = DXTexCoordX*OneOverDx;
    Result.XTexCoordsYStep = DXTexCoordY*OneOverDy;

    real32 DYTexCoordX = DInterpolant(Result.YTexCoords, Ver0.Pos.Y, Ver1.Pos.Y, Ver2.Pos.Y);
    real32 DYTexCoordY = DInterpolant(Result.YTexCoords, Ver0.Pos.X, Ver1.Pos.X, Ver2.Pos.X);
    Result.YTexCoordsXStep = DYTexCoordX*OneOverDx;
    Result.YTexCoordsYStep = DYTexCoordY*OneOverDy;

    real32 DOneOverZX = DInterpolant(Result.OneOverZ, Ver0.Pos.Y, Ver1.Pos.Y, Ver2.Pos.Y);
    real32 DOneOverZY = DInterpolant(Result.OneOverZ, Ver0.Pos.X, Ver1.Pos.X, Ver2.Pos.X);
    Result.OneOverZXStep = DOneOverZX*OneOverDx;
    Result.OneOverZYStep = DOneOverZY*OneOverDy;

    real32 DDepthX = DInterpolant(Result.Depth, Ver0.Pos.Y, Ver1.Pos.Y, Ver2.Pos.Y);
    real32 DDepthY = DInterpolant(Result.Depth, Ver0.Pos.X, Ver1.Pos.X, Ver2.Pos.X);
    Result.DepthXStep = DDepthX*OneOverDx;
    Result.DepthYStep = DDepthY*OneOverDy;

    real32 DLightAmountX = DInterpolant(Result.LightAmount, Ver0.Pos.Y, Ver1.Pos.Y, Ver2.Pos.Y);
    real32 DLightAmountY = DInterpolant(Result.LightAmount, Ver0.Pos.X, Ver1.Pos.X, Ver2.Pos.X);
    Result.LightAmountXStep = DLightAmountX*OneOverDx;
    Result.LightAmountYStep = DLightAmountY*OneOverDy;
    
    return Result;
}

edge
BuildEdge(v4 PosInit, v4 PosFin, gradients Gradients, int32 MinYIndex)
{
    edge Result = {};
    Result.YMin = (int32)(ceil(PosInit.Y));
    Result.YMax = (int32)(ceil(PosFin.Y));

    real32 YPrestep = Result.YMin - PosInit.Y;
    real32 XDisplacement = (PosFin.X - PosInit.X);
    real32 YDisplacement = (PosFin.Y - PosInit.Y);

    Result.XStep = (XDisplacement/YDisplacement);
    Result.X = PosInit.X + Result.XStep*YPrestep;

    real32 YStep = (YDisplacement/XDisplacement);
    real32 XPrestep = Result.X - PosInit.X;

    Result.XTexCoord = Gradients.XTexCoords[MinYIndex] + Gradients.XTexCoordsXStep*XPrestep + 
                        Gradients.XTexCoordsYStep*YPrestep;
    Result.XTexCoordStep = Gradients.XTexCoordsYStep + Gradients.XTexCoordsXStep*Result.XStep;

    Result.YTexCoord = Gradients.YTexCoords[MinYIndex] + Gradients.YTexCoordsXStep*XPrestep + 
                        Gradients.YTexCoordsYStep*YPrestep;
    Result.YTexCoordStep = Gradients.YTexCoordsYStep + Gradients.YTexCoordsXStep*Result.XStep;

    Result.OneOverZ = Gradients.OneOverZ[MinYIndex] + Gradients.OneOverZXStep*XPrestep +
                        Gradients.OneOverZYStep*YPrestep;
    Result.OneOverZStep = Gradients.OneOverZYStep + Gradients.OneOverZXStep*Result.XStep;

    Result.Depth = Gradients.Depth[MinYIndex] + Gradients.DepthXStep*XPrestep +
                        Gradients.DepthYStep*YPrestep;
    Result.DepthStep = Gradients.DepthYStep + Gradients.DepthXStep*Result.XStep;

    Result.LightAmount = Gradients.LightAmount[MinYIndex] + Gradients.LightAmountXStep*XPrestep +
                        Gradients.LightAmountYStep*YPrestep;
    Result.LightAmountStep = Gradients.LightAmountYStep + Gradients.LightAmountXStep*Result.XStep;
    
    return Result;
}

void 
CopyPixel(uint32 Source, int32 X, int32 Y, real32 LightAmount)
{
    real32 R = ((Source >> 16) & 0xFF)*LightAmount;
    real32 G = ((Source >> 8) & 0xFF)*LightAmount;
    real32 B = ((Source >> 0) & 0xFF)*LightAmount;
    uint32 *Pixel = (uint32 *)GlobalBackBuffer.Memory;

    Pixel[(int32)Y*GlobalBackBuffer.Width + X] = (0x00 << 24) | ((uint32)R << 16) | ((uint32)G << 8) | ((uint32)B << 0);
}

void
DrawHorizontalLine(gradients Gradients, int32 Y, edge Left, edge Right)
{
    int32 XMin = (int32)ceil(Left.X);
    int32 XMax = (int32)ceil(Right.X);
    real32 XPrestep = XMin - Left.X;
    real32 XDist = (Right.X - Left.X);

    real32 XTexCoordsXStep = (Right.XTexCoord - Left.XTexCoord)/XDist;
    real32 YTexCoordsXStep = (Right.YTexCoord - Left.YTexCoord)/XDist;
    real32 OneOverZXStep = (Right.OneOverZ - Left.OneOverZ)/XDist;
    real32 DepthXStep = (Right.Depth - Left.Depth)/XDist;
    real32 LightAmountXStep = (Right.LightAmount - Left.LightAmount)/XDist;

    real32 XTexCoord = Left.XTexCoord + Gradients.XTexCoordsXStep*XPrestep;
    real32 YTexCoord = Left.YTexCoord + Gradients.YTexCoordsXStep*XPrestep;
    real32 OneOverZ = Left.OneOverZ + Gradients.OneOverZXStep*XPrestep;
    real32 Depth = Left.Depth + Gradients.DepthXStep*XPrestep;
    real32 LightAmount = Left.LightAmount + Gradients.LightAmountXStep*XPrestep;

    int32 GlobalTextureHeight = GlobalTexture.Height;

    for(int32 X = XMin;
        X < XMax;
        X++)
    {
        if(Depth < ZBuffer.Memory[Y*ZBuffer.Width + X])
        {
            real32 Z = 1.0f/OneOverZ;
            ZBuffer.Memory[Y*ZBuffer.Width + X] = Depth;
            int32 XTexCoordInt = (int32)(((real32)GlobalTexture.Width - 1)*(XTexCoord*Z) + 0.5f);
            int32 YTexCoordInt = (int32)(((real32)GlobalTexture.Height - 1)*(YTexCoord*Z) + 0.5f);
            uint32 Source = GlobalTexture.Bitmap[YTexCoordInt*GlobalTextureHeight + XTexCoordInt];
            CopyPixel(Source, X, Y, LightAmount);
        }
        XTexCoord += XTexCoordsXStep;
        YTexCoord += YTexCoordsXStep;
        OneOverZ += OneOverZXStep;
        Depth += DepthXStep;
        LightAmount += LightAmountXStep;
    }
}

void
EdgeStep(edge *Edge)
{
    Edge->X += Edge->XStep;
    Edge->XTexCoord += Edge->XTexCoordStep;
    Edge->YTexCoord += Edge->YTexCoordStep;
    Edge->OneOverZ += Edge->OneOverZStep;
    Edge->Depth += Edge->DepthStep;
    Edge->LightAmount += Edge->LightAmountStep;
}

real32 
TriangleArea(v4 V1, v4 V2)
{
    real32 Result = 0.0f;
    Result = (V1.X*V2.Y - V1.Y*V2.X)*0.5f;
    return Result;
}

void
FillArea(edge *A, edge *B, gradients Gradients, bool32 Handedness)
{
    edge *Left = A;
    edge *Right = B;
    if(Handedness)
    {
        Right = A;
        Left = B;
    }

    int32 YMin = B->YMin;
    int32 YMax = B->YMax;

    for(int32 Y = YMin;
        Y < YMax;
        Y++)
    {
        DrawHorizontalLine(Gradients, Y, *Left, *Right);
        EdgeStep(Left);
        EdgeStep(Right);
    }
}

void
DrawTriangle(vertex Max, vertex Mid, vertex Min, bool32 Handedness)
{
    gradients Gradients = CreateGradients(Max, Mid, Min);
    edge MidToMax = BuildEdge(Mid.Pos, Max.Pos, Gradients, 1);
    edge MinToMax = BuildEdge(Min.Pos, Max.Pos, Gradients, 2);
    edge MinToMid = BuildEdge(Min.Pos, Mid.Pos, Gradients, 2);

    FillArea(&MinToMax, &MinToMid, Gradients, Handedness);
    FillArea(&MinToMax, &MidToMax, Gradients, Handedness);
}

void 
BuildTriangle(vertex Ver0, vertex Ver1, vertex Ver2, mat4 SSM)
{
    vertex MaxY = Ver0;
    MaxY.Pos = PerspectiveDivide(SSM*MaxY.Pos);
    vertex MidY = Ver1;
    MidY.Pos = PerspectiveDivide(SSM*MidY.Pos);
    vertex MinY = Ver2;
    MinY.Pos = PerspectiveDivide(SSM*MinY.Pos);

    if(MaxY.Pos.Y < MidY.Pos.Y)
    {
        vertex Temp = MaxY;
        MaxY = MidY;
        MidY = Temp;
    }
    if(MaxY.Pos.Y < MinY.Pos.Y)
    {
        vertex Temp = MaxY;
        MaxY = MinY;
        MinY = Temp;
    }
    if(MidY.Pos.Y < MinY.Pos.Y)
    {
        vertex Temp = MidY;
        MidY = MinY;
        MinY = Temp;
    } 

    v4 VMaxMid = (MidY.Pos - MaxY.Pos);
    v4 VMaxMin = (MinY.Pos - MaxY.Pos);
    real32 TriArea = TriangleArea(VMaxMid, VMaxMin);
    
    bool32 Handedness = 0; 

    if(TriArea > 0)
    {
        Handedness = 1;
    }

    DrawTriangle(MaxY, MidY, MinY, Handedness);
}

bool32 
IsVertexInScreen(v4 Pos)
{
    bool32 Result = true;
    for(int32 I = 0;
        I < 3;
        I++)
    {
        if((Pos.E[I] > Pos.W) || (Pos.E[I] < -Pos.W))
        {
            return false;
        }
    }
    return Result;
}

void 
ClipPolygon(vertex Ver0, vertex Ver1, vertex Ver2, mat4 SSM)
{
    bool32 Ver0InScreen = IsVertexInScreen(Ver0.Pos);
    bool32 Ver1InScreen = IsVertexInScreen(Ver1.Pos);
    bool32 Ver2InScreen = IsVertexInScreen(Ver2.Pos);
    if(Ver0InScreen &&
    Ver1InScreen &&
    Ver2InScreen) 
    {
        BuildTriangle(Ver0, Ver1, Ver2, SSM);
        return;
    }
    vertex Vertices[7] = {};
    Vertices[0] = Ver0;
    Vertices[1] = Ver1;
    Vertices[2] = Ver2;
    int32 Three = 3;
    int32 *VertAmount = &Three;

    vertex AuxVertices[7] = {};
    int32 Zero = 0;
    int32 *VertAuxAmount = &Zero;

    if(ClipVertexAxis(Vertices, VertAmount, AuxVertices, VertAuxAmount, 0) && 
    ClipVertexAxis(Vertices, VertAmount, AuxVertices, VertAuxAmount, 1) && 
    ClipVertexAxis(Vertices, VertAmount, AuxVertices, VertAuxAmount, 2))
    {
        for(int32 I = 1;
            I < ((*VertAmount) - 1);
            I++)
        {
            BuildTriangle(Vertices[0], Vertices[I], Vertices[I + 1], SSM);
        }
    }
}

void 
DrawMesh(mesh Mesh, mat4 Transform, mat4 NormalTransform, mat4 SSM)
{
    for(int32 I = 0;
        I < Mesh.VertexAmount;
        I += 3)
    {
        vertex Vec0T = Mesh.Vertices[I];
        Vec0T.Pos = Transform*Vec0T.Pos;
        Vec0T.Normal = NormalTransform*Vec0T.Normal;
        vertex Vec1T = Mesh.Vertices[I + 1];
        Vec1T.Pos = Transform*Vec1T.Pos;
        Vec1T.Normal = NormalTransform*Vec1T.Normal;
        vertex Vec2T = Mesh.Vertices[I + 2];
        Vec2T.Pos = Transform*Vec2T.Pos;
        Vec2T.Normal = NormalTransform*Vec2T.Normal;
        ClipPolygon(Vec0T, Vec1T, Vec2T, SSM);
    }
}

void 
FillBackground(back_buffer *Buffer)
{
    uint8 *Row = (uint8 *)Buffer->Memory;
    int32 Width = Buffer->Width;
    int32 Height = Buffer->Height;
    int32 Pitch = Buffer->Pitch;

    for(int32 Y = 0;
        Y < Height;
        Y++)
    {
        uint32 *Pixel = (uint32 *)Row;
        for(int32 X = 0;
            X < Width;
            X++)
        {
            uint8 R = 0x1F;
            uint8 G = 0x1F;
            uint8 B = 0x1F;
            *Pixel++ = (0x00 << 24) | (R << 16) | (G << 8) | (B << 0);
        }

        Row += Pitch;
    }
}

void 
CreateScreenBuffer(int32 Width, int32 Height, int32 BytesPerPixel)
{
    if(GlobalBackBuffer.Memory)
    {
        VirtualFree(GlobalBackBuffer.Memory, 0, MEM_RELEASE);
    }
    BITMAPINFOHEADER Header = {};

    Header.biSize = sizeof(Header);
    Header.biWidth = Width;
    Header.biHeight = Height;
    Header.biPlanes = 1;
    Header.biBitCount = 32;
    Header.biCompression = BI_RGB;

    BITMAPINFO BitmapInfo = {};
    BitmapInfo.bmiHeader = Header;

    GlobalBackBuffer.Info = BitmapInfo;
    GlobalBackBuffer.Width = Width;
    GlobalBackBuffer.Height = Height;
    GlobalBackBuffer.BytesPerPixel = BytesPerPixel;
    GlobalBackBuffer.Pitch = Width*GlobalBackBuffer.BytesPerPixel;

    int32 BufferSize = GlobalBackBuffer.Width*GlobalBackBuffer.Height*GlobalBackBuffer.BytesPerPixel;

    GlobalBackBuffer.Memory = VirtualAlloc(0, BufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void 
DisplayBufferInScreen(HDC DeviceContext, int32 Width, int32 Height)
{
    StretchDIBits(DeviceContext, 0, 0, Width, Height, 0, 0, Width, Height,
      GlobalBackBuffer.Memory, &GlobalBackBuffer.Info, DIB_RGB_COLORS, SRCCOPY); 
}

LRESULT WindowCallback(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
    LRESULT Result = 0;
    switch(Message)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT Paint = {};
            HDC DeviceContext =  BeginPaint(Window, &Paint);
            DisplayBufferInScreen(DeviceContext, GlobalBackBuffer.Width, GlobalBackBuffer.Height);
            EndPaint(Window, &Paint);
        }break;       
        case WM_CLOSE:
        {
            GlobalRunning = false;
        }break;
        default:
        {
            Result = DefWindowProc(Window, Message, wParam, lParam);
        }
    }
    return Result;
}

void
ProcessMessages(MSG *Message, bool32 *MoveFoward, bool32 *MoveBack, 
                bool32 *MoveLeft, bool32 *MoveRight,
                bool32 *PointRight, bool32 *PointLeft,
                bool32 *PointUp, bool32 *PointDown,
                bool32 *RenderMonkey, bool32 *RenderSphere,
                bool32 *Speed, bool32 *Rotate, bool32 *Move)
{
    switch(Message->message)
    {
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYUP:
        case WM_KEYDOWN:
        {
            uint32 VKCode = Message->wParam;
            bool32 IsDown = (Message->lParam >> 31 == 0);
            bool32 IsUp = (Message->lParam >> 31 == 1);
            if(VKCode == 0x57) //W KEY
            {
                if(IsDown)
                {
                    *MoveFoward = true;
                }
                else
                {
                    *MoveFoward = false;
                }
            }
            if(VKCode == 0x53) //S KEY
            {
                if(IsDown)
                {
                    *MoveBack = true;
                }
                else
                {
                    *MoveBack = false;
                }
            }
            if(VKCode == 0x41) //A KEY
            {
                if(IsDown)
                {
                    *MoveLeft = true;
                }
                else
                {
                    *MoveLeft = false;
                }
            }
            if(VKCode == 0x44) //D KEY
            {
                if(IsDown)
                {
                    *MoveRight = true;
                }
                else
                {
                    *MoveRight = false;
                }
            }
            if(VKCode == VK_UP) 
            {
                if(IsDown)
                {
                    *PointUp = true;
                }
                else
                {
                    *PointUp = false;
                }
            }
            if(VKCode == VK_DOWN)
            {
                if(IsDown)
                {
                    *PointDown = true;
                }
                else
                {
                    *PointDown = false;
                }
            }
            if(VKCode == VK_RIGHT)
            {
                if(IsDown)
                {
                    *PointRight = true;
                }
                else
                {
                    *PointRight = false;
                }
            }
            if(VKCode == VK_LEFT)
            {
                if(IsDown)
                {
                    *PointLeft = true;
                }
                else
                {
                    *PointLeft = false;
                }
            }
            if(VKCode == 0x31) //1 KEY
            {
                *RenderMonkey = true;
                *RenderSphere = false;
            }
            if(VKCode == 0x32) //2 KEY
            {
                *RenderMonkey = false;
                *RenderSphere = true;
            }
            if(VKCode == 0x33) //3 KEY
            {
                *RenderMonkey = false;
                *RenderSphere = false;
            }
            if(VKCode == VK_SPACE) 
            {
                if(IsUp)
                {
                    if(*Speed)
                    {
                        *Speed = false;
                    }
                    else
                    {
                        *Speed = true;
                    }
                }
            }
            if(VKCode == 0x52) // R KEY 
            {
                if(IsUp)
                {
                    if(*Rotate)
                    {
                        *Rotate = false;
                    }
                    else
                    {
                        *Rotate = true;
                    }
                }
            }
            if(VKCode == 0x4D) // M KEY 
            {
                if(IsUp)
                {
                    if(*Move)
                    {
                        *Move = false;
                    }
                    else
                    {
                        *Move = true;
                    }
                }
            }
        }break;
        default:
        {
            TranslateMessage(Message); 
            DispatchMessage(Message); 
        }
    }
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    WNDCLASSEXA WindowClass = {};

    WindowClass.cbSize = sizeof(WindowClass);
    WindowClass.lpfnWndProc = WindowCallback;
    WindowClass.hInstance = hInstance;
    WindowClass.lpszClassName = "Window Class";
    

    if(RegisterClassExA(&WindowClass))
    {
        HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "My Winow", 
                        WS_OVERLAPPEDWINDOW|WS_VISIBLE, 
                        CW_USEDEFAULT, CW_USEDEFAULT, 
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        0, 0, hInstance, 0);

        CreateScreenBuffer(800, 600, 4);

        LARGE_INTEGER lpFrequency; 
        QueryPerformanceFrequency(&lpFrequency);
        int64 Frequency = lpFrequency.QuadPart;

        if(Window)
        {
            read_file_result CubeFile = ReadOBJFile("cube.obj");
            mesh CubeMesh = OBJFileToMesh(CubeFile);

            read_file_result MonkeyFile = ReadOBJFile("monkey.obj");
            mesh MonkeyMesh = OBJFileToMesh(MonkeyFile);

            read_file_result SphereFile = ReadOBJFile("icosphere.obj");
            mesh SphereMesh = OBJFileToMesh(SphereFile);

            GlobalRunning = true;
            
            real32 HalfScreenWidth = (real32)GlobalBackBuffer.Width/2.0f;
            real32 HalfScreenHeight = (real32)GlobalBackBuffer.Height/2.0f;

            mat4 Identity = {};
            Identity.E[0][0] = 1.0f; Identity.E[0][1] = 0.0f; Identity.E[0][2] = 0.0f; Identity.E[0][3] = 0.0f;
            Identity.E[1][0] = 0.0f; Identity.E[1][1] = 1.0f; Identity.E[1][2] = 0.0f; Identity.E[1][3] = 0.0f;
            Identity.E[2][0] = 0.0f; Identity.E[2][1] = 0.0f; Identity.E[2][2] = 1.0f; Identity.E[2][3] = 0.0f;
            Identity.E[3][0] = 0.0f; Identity.E[3][1] = 0.0f; Identity.E[3][2] = 0.0f; Identity.E[3][3] = 1.0f;

            mat4 SSM = {};
            SSM.E[0][0] = HalfScreenWidth; SSM.E[0][1] = 0.0f;             SSM.E[0][2] = 0.0f; SSM.E[0][3] = (HalfScreenWidth - 0.5f);
            SSM.E[1][0] = 0.0f;            SSM.E[1][1] = HalfScreenHeight; SSM.E[1][2] = 0.0f; SSM.E[1][3] = (HalfScreenHeight - 0.5f);
            SSM.E[2][0] = 0.0f;            SSM.E[2][1] = 0.0f;             SSM.E[2][2] = 1.0f; SSM.E[2][3] = 0.0f;
            SSM.E[3][0] = 0.0f;            SSM.E[3][1] = 0.0f;             SSM.E[3][2] = 0.0f; SSM.E[3][3] = 1.0f;

            real32 AspectRatio = (real32)GlobalBackBuffer.Width/(real32)GlobalBackBuffer.Height;
            mat4 Projection = InitPerspectiveProjection(0.7853981f, AspectRatio, 0.1f, 100.0f);


            ZBuffer.Width = GlobalBackBuffer.Width;
            ZBuffer.Height = GlobalBackBuffer.Height;
            int32 ZBufferSize = ZBuffer.Width*ZBuffer.Height*4;
            ZBuffer.Memory = (real32 *)VirtualAlloc(0, ZBufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

            int32 Texels[32][32] = {};
            for(int j = 0;
                j < 32;
                j++)
            {
                for(int i = 0;
                    i < 32;
                    i++)
                {
                    uint8 R = (uint8)(rand() % 255);
                    uint8 G = (uint8)(rand() % 255);
                    uint8 B = (uint8)(rand() % 255);
                    Texels[j][i] = (0x00 << 24) | (R << 16) | (G << 8) | (B << 0);
                }
            }

            GlobalTexture.Bitmap = (uint32 *)Texels;
            GlobalTexture.Width = 32;
            GlobalTexture.Height = 32;

            real32 Angle = 0.0f;
            real32 Distance = 0.0f;

            v4 CameraPos = {0.0f, 0.0f, 5.0f, 0.0f};
            v4 CameraFront = {0.0f, 0.0f, 0.0f, 0.0f};
            v4 CameraUp = {0.0f, 1.0f, 0.0f, 0.0f};

            mat4 View = Identity;
            bool32 MoveFoward = false; 
            bool32 MoveBack = false; 
            bool32 MoveLeft = false; 
            bool32 MoveRight = false;
            bool32 PointRight = false;
            bool32 PointLeft = false;
            bool32 PointUp = false;
            bool32 PointDown = false;
            bool32 RenderMonkey = false;
            bool32 RenderSphere = false;
            bool32 Speed = false;
            bool32 Rotate = false;
            bool32 Move = false;
            real32 Yaw = -1.570796327f;
            real32 Pitch = 0.0f;
            real32 CameraSpeed = 0.0f;

            LARGE_INTEGER lpPerformanceCount;
            QueryPerformanceCounter(&lpPerformanceCount);       
            uint64 LastCount = lpPerformanceCount.QuadPart;

            while(GlobalRunning)
            {
                MSG Message; 
                while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                {
                    ProcessMessages(&Message, &MoveFoward, &MoveBack, &MoveLeft, 
                                    &MoveRight, &PointRight, &PointLeft, 
                                    &PointUp, &PointDown,
                                    &RenderMonkey, &RenderSphere, 
                                    &Speed, &Rotate, &Move);
                }
                if(MoveFoward)
                {
                    CameraPos += CameraFront*CameraSpeed*0.7f;
                }
                if(MoveBack)
                {
                    CameraPos -= CameraFront*CameraSpeed*0.7f;
                }
                if(MoveRight)
                {
                    CameraPos += Normalize(CrossProduct(CameraFront, CameraUp))*CameraSpeed*0.7f;
                }
                if(MoveLeft)
                {
                    CameraPos -= Normalize(CrossProduct(CameraFront, CameraUp))*CameraSpeed*0.7f;
                }
                if(PointRight)
                {
                    Yaw += CameraSpeed*0.1f;
                }
                if(PointLeft)
                {
                    Yaw -= CameraSpeed*0.1f;
                }
                if(PointUp)
                {
                    Pitch += CameraSpeed*0.1f;
                }
                if(PointDown)
                {
                    Pitch -= CameraSpeed*0.1f;
                }

                CameraFront.X = cosf(Yaw)*cosf(Pitch);
                CameraFront.Y = sinf(Pitch);
                CameraFront.Z = sinf(Yaw)*cosf(Pitch);

                v4 Aux = {-CameraFront.Z, 0.0f, CameraFront.X, 0.0f};
                CameraUp = Normalize(CrossProduct(Aux, CameraFront));

                FillBackground(&GlobalBackBuffer);
                ClearZBuffer();
                View = CreateLookAt(CameraPos, CameraPos + CameraFront, CameraUp);
                
                for(int32 I = 0;
                    I < 3;
                    I++)
                {
                    mat4 Translation = InitTranslation(-3.0f + (real32)I*3.0f, (real32)I, -5.0f + 3.0f*cosf(Distance));
                    mat4 Rotation = InitRotation(0.0f, Angle, 0.0f);
                    mat4 Model = Translation*Rotation;
                    mat4 Transform = Projection*View*Model;
                    mat4 NormalTransform = Projection*Model;
                    if(RenderMonkey)
                    {
                        DrawMesh(MonkeyMesh, Transform, NormalTransform, SSM);
                    }
                    else if(RenderSphere)
                    {

                        DrawMesh(SphereMesh, Transform, NormalTransform, SSM);
                    }
                    else
                    {
                        DrawMesh(CubeMesh, Transform, NormalTransform, SSM);
                    }
                }

                HDC DeviceContext = GetDC(Window);
                DisplayBufferInScreen(DeviceContext, GlobalBackBuffer.Width, 
                        GlobalBackBuffer.Height);
                ReleaseDC(Window, DeviceContext);

                LARGE_INTEGER PerformanceCount;
                QueryPerformanceCounter(&PerformanceCount);       
                uint64 EndCount = PerformanceCount.QuadPart;
                real32 msPerFrame = (EndCount - LastCount)*1000 / (real32)Frequency;
                LastCount = EndCount;

                if(Rotate)
                {
                    Angle += (msPerFrame/2000.0f);
                }
                if(Move)
                {
                    Distance += (msPerFrame/2000.0f); 
                }
                if(Angle > 3.1415f*2.0f)
                {
                    Angle = 0.0f;
                }
                if(Yaw > 3.1415f*2.0f)
                {
                    Yaw = 0.0f;
                }
                if(Pitch > 1.570796327f)
                {
                    Yaw = 1.570796327f;
                }
                if(Pitch < -1.570796327f)
                {
                    Yaw = -1.570796327f;
                }
                if(Speed)
                {
                    CameraSpeed = (msPerFrame/1000.0f)*10.0f;
                }
                else
                {
                    CameraSpeed = (msPerFrame/1000.0f)*3.0f;
                }
            }
        }
    } 

    return 0;
}
