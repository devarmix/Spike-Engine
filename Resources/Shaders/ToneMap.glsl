vec3 Uncharted2Tonemap(vec3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;

    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F)) - E/F;
}

vec3 ToneMap(vec3 color) {

    float exposure = 3.f;
    color = Uncharted2Tonemap(color * exposure);
    float whitePoint = 11.2f;

    vec3 whiteScale = 1.0f / Uncharted2Tonemap(vec3(whitePoint));
    color *= whiteScale;

    return color;
}