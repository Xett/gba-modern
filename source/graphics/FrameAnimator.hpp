//--------------------------------------------------------------------------------
// FrameAnimator.hpp
//--------------------------------------------------------------------------------
// An animator that places all the frames in the VRAM and changes the tile
// IDs to do the animation
//--------------------------------------------------------------------------------

#pragma once

#include <tonc.h>
#include <cstdint>
#include <utility>
#include "graphics.hpp"
#include "graphics/AnimationPose.hpp"
#include "graphics/SpriteSize.hpp"
#include "util/AllocatorPointer.hpp"
#include "util/integer-sequence-utils.hpp"

// Contains the base of the frame animator, used to be able to share code
class FrameAnimatorBase final
{
    // The pointer to the tile indices
    const u16* frameTileIndices;
    // The number of GBA frames to wait between each animation frame
    u16 frameTime;
    // The GBA count, and important properties of the animation
    u16 frameCount, curFrame, repeatFrame, endFrame;

public:
    // Constructor and destructor
    FrameAnimatorBase(const u16* frameTileIndices, u16 frameTime);
    ~FrameAnimatorBase() {}

    // Sets an animation pose
    void setAnimationPose(const AnimationPose& pose);

    // Update the animation
    void update();

    // Gets the animation tile ID for that day
    u16 getTileId() const { return frameTileIndices[curFrame]; }
};

// Provides a place where we can put all the graphics
template <typename T, SpriteSize> class FrameStore;
template <std::size_t... Szs, SpriteSize Size>
class FrameStore<std::index_sequence<Szs...>, Size>
    : public Allocator<FrameStore<std::index_sequence<Szs...>, Size>>
{
    // Useful typedefs
    using Indices = std::index_sequence<Szs...>;
    using Sizes = exp2_seq_t<std::index_sequence<Szs...>>;
    using PrefixSizes = prefix_sum_t<Sizes>;
    static constexpr auto TotalSize = seq_sum_v<Sizes>;

    // Dirty technique to allow partial specialization of functions
    template <typename> struct dummy {};

    // The pointer where the animation frames are stored
    const TILE* animationFrames;

    // The places to store the tile indices
    u16 tileIndices[TotalSize];

    // The clear function
    template <std::size_t... Ofs>
    void clearUtil(dummy<std::index_sequence<Ofs...>>)
    {
        ((void)graphics::freeObjTiles(tileIndices[Ofs]), ...);
    }
    void clear() { clearUtil(dummy<PrefixSizes>{}); }

    // The alloc function is a bit more difficult, but doable nevertheless
    template <std::size_t Order, std::size_t N>
    void allocSingle()
    {
        // Set the allocation function
        constexpr auto SprSize = SizeUtils::logBlocks(Size);
        constexpr auto LogBlocks = SprSize + Order;
        tileIndices[N] = graphics::allocateObjTiles(LogBlocks);
        for (uint i = 1; i < (1 << Order); i++)
            tileIndices[N+i] = tileIndices[N] + (i << SprSize);

        // Transfer the data
        auto frame = animationFrames + (N << SprSize);
        auto dest = &tile_mem_obj[0][tileIndices[N]];
        memcpy32(dest, frame, (sizeof(TILE)/sizeof(u32)) << LogBlocks);
    }

    template <std::size_t... Orders, std::size_t... Ns>
    void allocUtil(dummy<std::index_sequence<Orders...>>, dummy<std::index_sequence<Ns...>>)
    {
        ((void)allocSingle<(std::size_t)Orders, (std::size_t)Ns>(), ...);
    }
    void alloc() { allocUtil(dummy<Indices>{}, dummy<PrefixSizes>{}); }

public:
    FrameStore(const void* animationFrames)
        : animationFrames((const TILE*)animationFrames), tileIndices() {};
    ~FrameStore() {}

    const u16* getFrameStore() const { return tileIndices; }

    template <typename T>
    friend class Allocator;
};

// The pointer for the frame store
template <typename T, SpriteSize Size>
class FrameStorePointer final
    : public AllocatorPointer<FrameStore<T, Size>>
{
public:
    INHERIT_ALLOCATOR_CTORS(FrameStorePointer, FrameStore<T, Size>)

    const u16* getFrameStore() const
    {
        ASSERT(this->allocator);
        return this->allocator->getFrameStore();
    }
};

// The frame animator, to be used with a special definition generated by the tools
template <typename T, SpriteSize Size>
class FrameAnimator
{
    FrameStorePointer<T, Size> pointer;
    FrameAnimatorBase base;

public:
    FrameAnimator(FrameStore<T, Size>& store, u16 frameTime)
        : pointer(store, false), base(store.getFrameStore(), frameTime) {}

    // Sets an animation pose
    void setAnimationPose(const AnimationPose& pose) { base.setAnimationPose(pose); }

    // Update the animation
    void update() { base.update(); }

    // Gets the animation tile ID for that day
    u16 getTileId() const { return base.getTileId(); }

    bool isVisible() const { return (bool)pointer; }
    void setVisible(bool visible) { pointer.setActive(visible); }
};
