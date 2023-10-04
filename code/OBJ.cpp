#pragma once

read_file_result 
ReadOBJFile(char *FileName)
{
    read_file_result Result = {};
    HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, 0, 0, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize64;
        if(GetFileSizeEx(FileHandle, &FileSize64))
        {
            uint32 FileSize = FileSize64.LowPart;
            void *Buffer = VirtualAlloc(0, FileSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            DWORD BytesRead;
            if(ReadFile(FileHandle, Buffer, FileSize, &BytesRead, 0) && FileSize == BytesRead)
            {
                Result.Contents = Buffer;
                Result.ContentsSize = FileSize;
            }

        }
        
        CloseHandle(FileHandle);
    }
    return Result;
}


void
ClearMemoryToZero(void *Buffer, int32 Size)
{
    uint8 *Pointer = (uint8 *)Buffer;
    for(int32 I = 0;
        I < Size;
        I++)
    {
        *Pointer++ = 0;
    }
}

real32 
GenerateComponent(uint8 **Char)
{
    real32 Result = 0;
    int32 IntegerPart = 0;
    real32 DecimalPart = 0;
    int32 DecimalCount = 0;
    bool32 IsNegative = false;
    bool32 GeneratingDecimalPart = false;
    if(**Char == '-')
    {
        IsNegative = true;
        (*Char)++;
    }
    while((**Char != ' ') && (**Char != '\r') && (**Char != '\n'))
    {
        if(**Char == '.')
        {
            GeneratingDecimalPart = true;
            (*Char)++;
        }
        if(GeneratingDecimalPart)
        {
            DecimalCount++;
            DecimalPart = (**Char - '0') + DecimalPart*10;
        }
        else
        {
            IntegerPart = (**Char - '0') + IntegerPart*10;
        }
        (*Char)++;
    }
    Result = (real32)IntegerPart + DecimalPart/(real32)Power(10, DecimalCount);
    if(IsNegative)
    {
        Result = -Result;
    }
    return Result;
}

void
NextLine(uint8 **Char)
{
    while((**Char) != '\n')
    {
        (*Char)++;
    }
    (*Char)++;
}

void
NextWord(uint8 **Char)
{
    while(((**Char) != ' ') && ((**Char) != '\n'))
    {
        (*Char)++;
    }
    (*Char)++;
}

void
AddPosition(uint8 *Char, v4 *Position)
{
    NextWord(&Char);
    Position->X = GenerateComponent(&Char);
    NextWord(&Char);
    Position->Y = GenerateComponent(&Char);
    NextWord(&Char);
    Position->Z = GenerateComponent(&Char);
    Position->W = 1.0f;
}

void
AddTexCoords(uint8 *Char, v4 *TexCoords)
{
    NextWord(&Char);
    TexCoords->X = GenerateComponent(&Char);
    NextWord(&Char);
    TexCoords->Y = GenerateComponent(&Char);
    TexCoords->Z = 0.0f; 
    TexCoords->W = 0.0f;
}

void
AddNormal(uint8 *Char, v4 *Normal)
{
    NextWord(&Char);
    Normal->X = GenerateComponent(&Char);
    NextWord(&Char);
    Normal->Y = GenerateComponent(&Char);
    NextWord(&Char);
    Normal->Z = GenerateComponent(&Char);
    Normal->W = 0.0f;
}

void
AddVertexToMesh(uint8 *Char, vertex *Vertex, v4 *Position, v4 *TexCoords, v4 *Normal)
{
    Char += 2;
    for(int32 I = 0;
        I < 3;
        I++)
    {
        int32 PositionIndex = 0;
        int32 TexCoordsIndex = 0;
        int32 NormalIndex = 0;

        while(*Char != '/')
        {
            PositionIndex = (*Char - '0') + PositionIndex*10;
            Char++;
        }

        Char++;
        while(*Char != '/')
        {
            TexCoordsIndex = (*Char - '0') + TexCoordsIndex*10;
            Char++;
        }

        Char++;
        while((*Char != ' ') && (*Char != '\r') && (*Char != '\n'))
        {
            NormalIndex = (*Char - '0') + NormalIndex*10;
            Char++;
        }

        Vertex->Pos = Position[PositionIndex - 1];
        Vertex->TexCoords = TexCoords[TexCoordsIndex - 1];
        Vertex->Normal = Normal[NormalIndex - 1];
        Vertex++;

        if(I < 2)
        {
            NextWord(&Char);
        }
    }
}

int32
CountLines(uint8 *Char, uint32 FileSize)
{
    int32 Result = 0;
    for(uint32 I = 0;
        I < FileSize;
        I++)
    {
        if(*Char == '\n')
        {
            Result++;
        }
        Char++;
    }
    return Result;
}

mesh
OBJFileToMesh(read_file_result File)
{
    mesh Result = {};
    void *DataBuffer = File.Contents;
    uint32 FileSize = File.ContentsSize;
    uint8 *Char = (uint8 *)DataBuffer;
    int32 TotalLines = CountLines(Char, FileSize);
    int32 VerticesPosAmount = 0;
    int32 VerticesNormalAmount = 0;
    int32 VerticesTexCoordsAmount = 0;
    int32 TrianglesAmount = 0;

    for(int32 Line = 0;
        Line < TotalLines;
        Line++)
    {
        if(*Char == 'v' && *(Char + 1) == ' ')
        {
            VerticesPosAmount++;
        }
        if(*Char == 'v' && *(Char + 1) == 'n')
        {
            VerticesNormalAmount++;
        }
        if(*Char == 'v' && *(Char + 1) == 't')
        {
            VerticesTexCoordsAmount++;
        }
        if(*Char == 'f')
        {
            TrianglesAmount++;
        }
        NextLine(&Char);
    }

    int32 VectorSize = sizeof(v4);
    int32 VertexSize = sizeof(vertex);
    int32 VertexPerFace = 3;
    void *VerticesPosition = malloc(VerticesPosAmount*VectorSize);
    void *VerticesTexCoords = malloc(VerticesTexCoordsAmount*VectorSize);
    void *VerticesNormal = malloc(VerticesNormalAmount*VectorSize);
    void *Vertices = malloc(TrianglesAmount*VertexSize*VertexPerFace);
    ClearMemoryToZero(VerticesPosition, VerticesPosAmount*VectorSize);
    ClearMemoryToZero(VerticesTexCoords, VerticesTexCoordsAmount*VectorSize);
    ClearMemoryToZero(Vertices, TrianglesAmount*VertexSize*VertexPerFace);
    
    Result.VertexAmount = TrianglesAmount*VertexPerFace;
    v4 *PositionAux = (v4 *)VerticesPosition;
    v4 *TexCoordsAux = (v4 *)VerticesTexCoords;
    v4 *NormalAux = (v4 *)VerticesNormal;

    vertex *Vertex = (vertex *)Vertices;
    v4 *Position = (v4 *)VerticesPosition;
    v4 *TexCoords = (v4 *)VerticesTexCoords;
    v4 *Normal = (v4 *)VerticesNormal;
    

    Char = (uint8 *)DataBuffer;
    for(int32 Line = 0;
        Line < TotalLines;
        Line++)
    {
        if(*Char == 'v' && *(Char + 1) == ' ')
        {
            AddPosition(Char, PositionAux); 
            PositionAux++;
        }
        else if(*Char == 'v' && *(Char + 1) == 'n')
        {
            AddNormal(Char, NormalAux); 
            NormalAux++;
        }
        else if(*Char == 'v' && *(Char + 1) == 't')
        {
            AddTexCoords(Char, TexCoordsAux); 
            TexCoordsAux++;
        }
        else if(*Char == 'f')
        {
            AddVertexToMesh(Char, Vertex, Position, TexCoords, Normal);
            Vertex += 3;
        }
        NextLine(&Char);
    }
    free(VerticesPosition);
    free(VerticesTexCoords);
    free(VerticesNormal);
    Result.Vertices = (vertex *)Vertices;
    return Result;
}
