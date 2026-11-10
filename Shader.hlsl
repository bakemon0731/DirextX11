cbuffer ConstantBuffer : register(b0)
{
    matrix g_World;
    matrix g_View;
    matrix g_Projection;
}

struct VS_INPUT
{
    float4 pos : POSITION;
    float3 normal : NORMAL;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 normal : TEXCOORD0;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;

    // World -> View -> Projection transform
    float4 worldPos = mul(input.pos, g_World);
    float4 viewPos = mul(worldPos, g_View);
    output.pos = mul(viewPos, g_Projection);

    // Transform the normal so lighting follows the rotation
    output.normal = mul(input.normal, (float3x3) g_World);

    return output;
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    // Directional light from the upper right
    float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
    float3 normal = normalize(input.normal);

    // Brightness from the angle between surface and light
    float lightIntensity = saturate(dot(normal, lightDir));

    // Ambient light (prevents pure black)
    float ambient = 0.2f;
    float finalBrightness = lightIntensity + ambient;

    // Multiply the base red color by the brightness
    float4 baseColor = float4(1.0f, 0.0f, 0.0f, 1.0f);
    return baseColor * finalBrightness;
}

