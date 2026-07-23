#pragma once
#include <ShitEngine.h>

class Player : public Shit::Behavior {
    private:
        Shit::TransformComponent* transform {nullptr};
        float speed {200.0f};

    public:
        void onStart() override;
        void onUpdate() override;
    
};