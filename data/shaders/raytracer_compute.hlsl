#define PI 3.1415926f
#define SAMPLES 50

struct Ray {
    float3 origin;
    float3 direction;
};

struct Image {
    float width;
    float height;
};

struct Camera {
    float aspect_ratio;
    float lens_radius;
    float3 origin, lower_left_corner, horizontal, vertical;
    float3 u, v, w;
};

struct Hit {
    float3 pos;
    float3 normal;
    float t;
};

struct Material {
    int type; // 0->emissive, 1->lambertian, 2->metal
    float3 albedo;
    float fuzziness;
};

struct Sphere {
    float3 center;
    float radius;
};

struct Properties {
    int width;
    int height;
    int frame_count;
    int sphere_count;
    Camera camera;
};

RWTexture2D<float4> pixels : register(u0);
StructuredBuffer<Properties> properties_list : register(t0);
StructuredBuffer<Sphere> spheres : register(t1);
StructuredBuffer<Material> materials : register(t2);

uint wang_hash(inout uint seed) {
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

float random_float(inout uint state)
{
    return (wang_hash(state) & 0xFFFFFF) / 16777216.0f;
}

float random_float_between(inout uint state, float min, float max) {
    return min + (max - min) * random_float(state);
}

float3 random_in_unit_disk(inout uint state) {
    float a = random_float_between(state, 0, 2 * PI);
    float2 xy = float2(cos(a), sin(a)) * sqrt(random_float(state));
    return float3(xy, 0);
}

float3 random_in_unit_sphere(inout uint state) {
    float z = random_float_between(state, -1.0, 1.0);
    float t = random_float_between(state, 0.0, 2.0 * PI);
    float r = sqrt(max(0.0, 1.0f - z * z));
    float x = r * cos(t);
    float y = r * sin(t);
    return float3(x, y, z) * pow(random_float(state), 1.0 / 3.0);
}

float3 random_unit_vector(inout uint state) {
    float a = random_float_between(state, 0.0, 2.0 * PI);
    float z = random_float_between(state, -1.0, 1.0);
    float r = sqrt(1.0 - z * z);
    return float3(r * cos(a), r * sin(a), z);
}

Camera create_camera(float3 position, float3 look_at, float3 up, float aspect_r, float vertical_fov, float aperture, float focus_dist) {
    Camera camera;
    camera.aspect_ratio = aspect_r;

    float theta = radians(vertical_fov);
    float h = tan(theta / 2);
    float viewport_height = 2.0f * h;
    float viewport_width = camera.aspect_ratio * viewport_height;

    camera.w = normalize(position - look_at);
    camera.u = normalize(cross(up, camera.w));
    camera.v = normalize(cross(camera.w, camera.u));

    camera.origin = position;
    camera.horizontal = camera.u * viewport_width * focus_dist;
    camera.vertical = camera.v * viewport_height * focus_dist;
    camera.lower_left_corner = camera.origin - camera.horizontal / 2 - camera.vertical / 2 - camera.w * focus_dist;

    camera.lens_radius = aperture / 2;

    return camera;
}

float3 ray_at(Ray ray, float t) {
    return ray.origin + ray.direction * t;
}

Ray get_camera_ray(inout uint state, Camera cam, float u, float v) {
    float3 rd = random_in_unit_disk(state) * cam.lens_radius;
    float3 offset = cam.u * rd.x + cam.v * rd.y;
    
    Ray ray;
    ray.origin = cam.origin + offset;
    ray.direction = cam.lower_left_corner + cam.horizontal * u + cam.vertical * v - cam.origin - offset;

    return ray;
}

bool lambertian_scatter(inout uint state, Material material, Ray incoming_ray, Hit hit, out float3 attenuation, out Ray outgoing_ray) {
    float3 dir = hit.normal + random_unit_vector(state);
    
    Ray o_r;
    o_r.origin = hit.pos;
    o_r.direction = dir;

    outgoing_ray = o_r;
    attenuation = material.albedo;

    return true;
}

bool metal_scater(inout uint state, Material material, Ray incoming_ray, Hit hit, out float3 attenuation, out Ray outgoing_ray) {
    float3 reflected_vec = reflect(normalize(incoming_ray.direction), hit.normal);

    Ray o_r;
    o_r.origin = hit.pos;
    o_r.direction = reflected_vec + random_in_unit_sphere(state) * material.fuzziness;

    outgoing_ray = o_r;
    attenuation = material.albedo;

    return (dot(outgoing_ray.direction, hit.normal) > 0);
}

float3 emit(Material material) {
    if (material.type == 0) {
        return material.albedo;
    }

    return float3(0, 0, 0);
}

bool sphere_hit(Sphere sphere, Ray ray, float t_min, float t_max, inout Hit hit) {
    float3 diff = ray.origin - sphere.center;

    float a = dot(ray.direction, ray.direction);
    float b = dot(diff, ray.direction);
    float c = dot(diff, diff) - sphere.radius * sphere.radius;

    float discriminant = b * b - a * c;

    if (discriminant > 0) {
        float discriminant_sqrt = sqrt(discriminant);
        float first_root = (-b - discriminant_sqrt) / a;

        if (first_root > t_min&& first_root < t_max) {
            hit.t = first_root;
            hit.pos = ray_at(ray, hit.t);
            float3 outward_normal = (hit.pos - sphere.center) / sphere.radius;
            hit.normal = dot(ray.direction, outward_normal) < 0 ? outward_normal : -outward_normal;
            return true;
        }

        float second_root = (-b + discriminant_sqrt) / a;
        if (second_root > t_min&& second_root < t_max) {
            hit.t = second_root;
            hit.pos = ray_at(ray, hit.t);
            float3 outward_normal = (hit.pos - sphere.center) / sphere.radius;
            hit.normal = dot(ray.direction, outward_normal) < 0 ? outward_normal : -outward_normal;
            return true;
        }
    }
    return false;
}

bool scatter(inout uint state, Material material, Ray incoming_ray, Hit hit, inout float3 attenuation, inout Ray outgoing_ray) {
    if (material.type == 1) {
        return lambertian_scatter(state, material, incoming_ray, hit, attenuation, outgoing_ray);
    }
    else if (material.type == 2) {
        return metal_scater(state, material, incoming_ray, hit, attenuation, outgoing_ray);
    }

    return false;
}

int check_object_hit(int sphere_count, Ray ray, float t_min, float t_max, inout Hit hit) {
    Hit closest_hit;
    int selected_index = -1;
    float closest_hit_distance = t_max;

    for (int i = 0; i < sphere_count; i++) {
        if (sphere_hit(spheres[i], ray, t_min, closest_hit_distance, closest_hit)) {
            selected_index = i;
            closest_hit_distance = closest_hit.t;
        }
    }

    if (selected_index != -1) {
        hit = closest_hit;
    }

    return selected_index;
}

float3 trace_ray(inout uint state, int sphere_count, Ray ray) {
    float3 result = 0;
    float3 cumilative_attenuation = float3(1.0, 1.0, 1.0);

    for (int depth = 0; depth < 50; depth++) {
        Hit hit;
        int obj_index = check_object_hit(sphere_count, ray, 0.001f, 1.0e7f, hit);
        if (obj_index != -1) {
            Ray outgoing_ray;
            float3 attenuation;
            float3 emitted = emit(materials[obj_index]);

            if (scatter(state, materials[obj_index], ray, hit, attenuation, outgoing_ray)) {
                cumilative_attenuation *= attenuation;
                ray = outgoing_ray;
            }
            else {
                result += cumilative_attenuation * emitted;
                break;
            }
        }
        else {
            float t = 0.5 * normalize(ray.direction).y + 0.5;
            result += cumilative_attenuation * lerp(float3(1, 1, 1), float3(0.5, 0.7, 1.0), t);
            //result += cumilative_attenuation * float3(0, 0, 0);
            break;
        }
    }

    return result;
}

[numthreads(8, 8, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
    Properties properties = properties_list[0];
    Camera camera = properties.camera;
    float3 color = 0;
    uint random_state = (id.x * 1973 + id.y * 9277 + properties.frame_count * 26699) | 1;

    for (int i = 0; i < SAMPLES; i++) {
        float u = float(id.x + random_float(random_state)) / float(properties.width);
        float v = (properties.height - float(id.y + random_float(random_state))) / float(properties.height);
        Ray ray = get_camera_ray(random_state, camera, u, v);
        color += trace_ray(random_state, properties.sphere_count, ray);
    }

    color /= float(SAMPLES);

    pixels[id.xy] = lerp(float4(color, 1), pixels[id.xy], float(properties.frame_count) / float(properties.frame_count + 1));
}