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
    float3 position = input.Pos;
    //float4 position = (input.Pos, 1.0f);
    
    position = mul(position, modelMatrix * viewMatrix * projectionMatrix); // *viewMatrix* modelMatrix);
    //position = mul(position, viewMatrix);
    //position = mul(position, modelMatrix);

    output.Pos = float4(position.x, position.y, position.z, 1.0f);
    //output.Pos = position;

    output.Color = input.Color;

    return output;
}