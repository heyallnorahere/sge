using SGE;
using SGE.Components;

namespace Template.Assets.Scripts
{
    public sealed class ExampleScript : Script
    {
        public void OnUpdate(Timestep ts)
        {
            if (!HasComponent<RigidBodyComponent>())
            {
                return;
            }

            float timestep = (float)ts;
            Vector2 force = (Speed * timestep, 0f);

            float modifier = 0f;
            modifier += Input.GetKey(KeyCode.D) ? 1f : 0f;
            modifier -= Input.GetKey(KeyCode.A) ? 1f : 0f;

            var rigidbody = GetComponent<RigidBodyComponent>();
            rigidbody.AddForce(force * modifier);
        }

        public float Speed { get; set; } = 500f;
    }
}