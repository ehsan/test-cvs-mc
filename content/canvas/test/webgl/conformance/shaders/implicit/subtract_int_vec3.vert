// implicit cast of int to vec3 in subtraction should fail
void main()
{
    vec3 f = vec3(1.0, 2.0, 3.0) - 1;
}
