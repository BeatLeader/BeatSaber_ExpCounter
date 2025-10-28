using System.Collections.Generic;

namespace ExpCounter.Utils
{
    internal class ExperienceUtils
    {
        private static int ExperiencePerSecond = 9;

        static List<(double, double)> accMult = new List<(double, double)> {
                (1.00, 1.00),
                (0.95, 1.00),
                (0.935,0.983),
                (0.92, 0.948),
                (0.90, 0.89),
                (0.85, 0.80),
                (0.80, 0.73),
                (0.75, 0.69),
                (0.70, 0.67),
                (0.65, 0.65),
                (0.60, 0.60),
                (0.55, 0.515),
                (0.50, 0.44),
                (0.45, 0.38),
                (0.40, 0.34),
                (0.35, 0.315),
                (0.30, 0.30),
                (0.00, 0.30)};

        internal static float CalculateDuration(float prev, float next, float timeScale = 1f)
        {
            if (next - prev <= 1)
            {
                return (next - prev) / timeScale;
            }
            else
            {
                return 1f / timeScale;
            }
        }

        internal static float CalculateExp(float duration, float accuracy)
        {
            var accMult = CalculateMultFromAccuracy(accuracy);
            return ExperiencePerSecond * duration * accMult;
        }

        internal static float CalculateMultFromAccuracy(float value)
        {
            if (accMult[0].Item1 <= value) return (float)accMult[0].Item2;
            if (accMult[17].Item1 >= value) return (float)accMult[17].Item2;

            int i = 0;
            for (; i < accMult.Count; i++)
            {
                if (accMult[i].Item1 <= value)
                {
                    break;
                }
            }

            if (i == accMult.Count) return 0;

            if (i == 0)
            {
                i = 1;
            }

            double middle_dis = (value - accMult[i - 1].Item1) / (accMult[i].Item1 - accMult[i - 1].Item1);
            return (float)(accMult[i - 1].Item2 + middle_dis * (accMult[i].Item2 - accMult[i - 1].Item2));
        }   
    }
}
