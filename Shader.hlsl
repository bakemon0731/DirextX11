cbuffer ConstantBuffer : register(b0)
{
    matrix g_World;
    matrix g_View;
    matrix g_Projection;
}

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
    float3 worldPos : TEXCOORD0; // ピクセルごとのワールド座標
    float3 normal : TEXCOORD1; // 法線
    float2 uv : TEXCOORD2; // UV座標
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    float4 worldPos = mul(input.pos, g_World);
    output.worldPos = worldPos.xyz; // ワールド座標を保持してピクセルシェーダーへ送る

    float4 viewPos = mul(worldPos, g_View);
    output.pos = mul(viewPos, g_Projection);

    output.normal = mul(input.normal, (float3x3) g_World);
    output.uv = input.uv;
    return output;
}

// ----------------------------------------------------
// プレイヤー用ピクセルシェーダー（スペキュラーハイライト追加版）
// ----------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_Target
{
    float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f)); // 光の方向
    float3 normal = normalize(input.normal);

    //  ディフューズ（拡散反射）
    float diff = saturate(dot(normal, lightDir));
    float ambient = 0.2f;

    // スペキュラーハイライト（Blinn-Phong反射）
    // C++側のカメラ位置 (0.0f, 2.0f, -4.0f) と一致させる
    float3 eyePos = float3(0.0f, 2.0f, -4.0f);
    float3 viewDir = normalize(eyePos - input.worldPos); // 視線ベクトル
    
    // 光ベクトルと視線ベクトルの「ハーフベクトルの計算」
    float3 halfDir = normalize(lightDir + viewDir);
    
    // 32.0f は「ツヤの鋭さ（光沢度）」。数字が大きいほどピンポイントで鋭く光る。
    float spec = pow(saturate(dot(normal, halfDir)), 32.0f);

    // テクスチャから色を取得
    float4 texColor = g_Texture.Sample(g_Sampler, input.uv);
    
    // 最終カラー合成 = (テクスチャ * 明るさ) + スペキュラー（白い反射光）
    float3 finalColor = texColor.rgb * (diff + ambient) + float3(1.0f, 1.0f, 1.0f) * spec;

    return float4(finalColor, texColor.a);
}

// ----------------------------------------------------
// 敵用ピクセルシェーダー（赤み強調 ＋ スペキュラー）
// ----------------------------------------------------
float4 PS_Enemy(VS_OUTPUT input) : SV_Target
{
    float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
    float3 normal = normalize(input.normal);

    float diff = saturate(dot(normal, lightDir));
    float ambient = 0.2f;

    float3 eyePos = float3(0.0f, 2.0f, -4.0f);
    float3 viewDir = normalize(eyePos - input.worldPos);
    float3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(saturate(dot(normal, halfDir)), 32.0f);

    float4 texColor = g_Texture.Sample(g_Sampler, input.uv);
    float3 finalColor = texColor.rgb * (diff + ambient) + float3(1.0f, 1.0f, 1.0f) * spec;

    // 敵は赤みを強調
    finalColor.r *= 1.4f;

    return float4(finalColor, texColor.a);
}

// ----------------------------------------------------
// モノクロ用ピクセルシェーダー（タイムアップ時）
// ----------------------------------------------------
float4 PS_Monochrome(VS_OUTPUT input) : SV_Target
{
    float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
    float3 normal = normalize(input.normal);
    float diff = saturate(dot(normal, lightDir));
    float ambient = 0.2f;

    float4 texColor = g_Texture.Sample(g_Sampler, input.uv);
    float3 color = texColor.rgb * (diff + ambient);

    float gray = dot(color, float3(0.299f, 0.587f, 0.114f));
    return float4(gray, gray, gray, texColor.a);
}