#include <metal_stdlib>
using namespace metal;

kernel void copy(
    texture2d<float,access::sample> src[[texture(0)]],
    texture2d<float,access::write> dst[[texture(1)]],
    constant uint2 &clip[[buffer(0)]],
    uint2 gid[[thread_position_in_grid]]) {
    
    if(gid.x<clip.x&&gid.y<clip.y) {
       constexpr sampler _sampler(filter::nearest, coord::pixel);
       dst.write(src.sample(_sampler,float2(gid)),gid);
    }
    else {
        dst.write(float4(0,0,0,1),gid);
    }
}
