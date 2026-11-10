cbuffer ConstantBuffer : register(b0)
{
    matrix g_World;
    matrix g_View;
    matrix g_Projection;
}

// テクスチャとサンプラーの定義
Texture2D g_Texture : register(t0);
SamplerState g_Sampler : register(s0);

struct VS_INPUT
{
    float4 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 normal : TEXCOORD0;
    float2 uv : TEXCOORD1;
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
    output.uv = input.uv;

    return output;
}

// プレイヤー用ピクセルシェーダー
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

    // 単色のかわりにテクスチャから色を取得
    float4 texColor = g_Texture.Sample(g_Sampler, input.uv);
    
    return texColor * finalBrightness;
}

// 敵用のピクセルシェーダー
float4 PS_Enemy(VS_OUTPUT input) : SV_Target
{
    float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
    float3 normal = normalize(input.normal);
    float lightIntensity = saturate(dot(normal, lightDir));
    float ambient = 0.2f;
    float finalBrightness = lightIntensity + ambient;

    // 単色のかわりにテクスチャから色を取得
    float4 texColor = g_Texture.Sample(g_Sampler, input.uv);
    float4 finalColor = texColor * finalBrightness;
    
    // 敵オブジェクトの色合いを赤寄りに補正するエフェクト
    finalColor.r *= 1.4f;
    return finalColor;
}

// ----------------------------------------------------
// ゲームオーバー/リザルト用（モノクロ・グレースケール化シェーダー）
// ----------------------------------------------------
float4 PS_Monochrome(VS_OUTPUT input) : SV_Target
{
    float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
    float3 normal = normalize(input.normal);
    float lightIntensity = saturate(dot(normal, lightDir));
    float ambient = 0.3f;
    float finalBrightness = lightIntensity + ambient;
    
    float4 texColor = g_Texture.Sample(g_Sampler, input.uv);
    float4 color = texColor * finalBrightness;
    
    // NTSC系数による正確な輝度（モノクロ）計算
    float gray = dot(color.rgb, float3(0.299f, 0.587f, 0.114f));
    
    // モノクロカラーを出力 (RGBをすべて gray に統一)
    return float4(gray, gray, gray, color.a);

}

