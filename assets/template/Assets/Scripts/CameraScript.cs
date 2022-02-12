using SGE;
using SGE.Components;

namespace Template.Assets.Scripts
{
    public sealed class CameraScript : Script
    {
        public void OnUpdate(Timestep ts)
        {
            if (Target is null)
            {
                return;
            }

            var targetTransform = Target.GetComponent<TransformComponent>();
            var transform = GetComponent<TransformComponent>();
            transform.Translation = targetTransform.Translation;
        }
        
        public Entity Target { get; set; } = null;
    }
}