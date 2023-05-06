cbuffer CameraBuffer : register(b0) {
    matrix viewMatrix;
    matrix projectionMatrix;
};


struct VS_INPUT {
    float3 Pos : POSITION;
    float4 Color : COLOR;
};

struct PS_INPUT {
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

PS_INPUT main(VS_INPUT input) {
    PS_INPUT output;

    matrix modelMatrix = matrix(1, 0, 0, 0,
                                0, 1, 0, 0,
                                0, 0, 1, 0,
                                0, 0, 0, 1);

    // Transform the vertex position by the view and projection matrices.
    float4 position = float4(input.Pos, 1.0f);
    matrix viewProjectionMatrix = mul(viewMatrix, projectionMatrix);
    output.Pos = mul(position, viewProjectionMatrix);

    // set the color
    output.Color = input.Color;

    return output;
}