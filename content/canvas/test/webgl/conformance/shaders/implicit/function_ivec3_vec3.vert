vec3 foo(vec3 f) {
  return f;
}

void main() {
  vec3 f = foo(ivec3(1, 2, 3));
}
