// Standard.hlsl

//----------------------------- VS_Main -----------------------------

struct VS_Input
{
    //...
};

struct VS_Output
{
    //...
};

VS_Output VS_Main(VS_Input input)
{
    VS_Output output;
    //...
    return output;
}

//----------------------------- PS_Main -----------------------------

float4 PS_Main(VS_Output input) : SV_TARGET
{
    //...
}