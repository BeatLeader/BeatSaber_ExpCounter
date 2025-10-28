using CountersPlus.Counters.Interfaces;
using CountersPlus.Custom;
using CountersPlus.Utils;
using ExpCounter.Utils;
using SiraUtil.Logging;
using System;
using TMPro;
using Zenject;

namespace ExpCounter.Counters
{
    public class ExpCounterController : ICounter
    {
        private float LastTime = 0f;
        private float Duration = 0;

        [Inject]
        private readonly ScoreController scoreController;
        [Inject]
        private readonly AudioTimeSyncController atsc;
        [Inject]
        private readonly RelativeScoreAndImmediateRankCounter relatCounter;

        private TMP_Text counterText = null!;

        private readonly SiraLog logger;
        private readonly CanvasUtility canvasUtility;
        private readonly CustomConfigModel settings;
        private GameplayCoreSceneSetupData sceneSetupData;

        public ExpCounterController(SiraLog logger, CanvasUtility canvasUtility, CustomConfigModel settings, GameplayCoreSceneSetupData sceneSetupData)
        {
            this.logger = logger;
            this.canvasUtility = canvasUtility;
            this.settings = settings;
            this.sceneSetupData = sceneSetupData;
        }

        public void CounterInit()
        {
            if (HasNullReferences() || !settings.Enabled)
                return;

            LastTime = 0f;
            Duration = 0;
            InitCounterText();

            scoreController.scoringForNoteStartedEvent += ExpCounter_ScoringForNoteStarted;
        }

        private void InitCounterText()
        {
            counterText = canvasUtility.CreateTextFromSettings(settings);

            RefreshCounterText(0);
        }

        public bool HasNullReferences()
        {
            if (canvasUtility == null || settings == null || sceneSetupData == null)
            {
                logger.Error("ExpCounter has a null reference and cannot initialize!");
                logger.Error("The following objects are null:");
                if (canvasUtility == null)
                    logger.Error("- CanvasUtility");
                if (settings == null)
                    logger.Error("- Settings");
                if (sceneSetupData == null)
                    logger.Error("- sceneSetupData");

                return true;
            }

            return false;
        }

        private void ExpCounter_ScoringForNoteStarted(ScoringElement scoringElement)
        {
            if (scoringElement is MissScoringElement && scoringElement.noteData.scoringType == NoteData.ScoringType.NoScore) return;

            if (scoringElement.time > LastTime)
            {
                Duration += ExperienceUtils.CalculateDuration(LastTime, scoringElement.time, atsc.timeScale);
                LastTime = scoringElement.time;
                RefreshCounterText(ExperienceUtils.CalculateExp(Duration, relatCounter.relativeScore));
            }
        }

        private void RefreshCounterText(float exp)
        {
            counterText.text = Math.Round(exp).ToString() + "xp";
        }

        public void CounterDestroy()
        {
            scoreController.scoringForNoteStartedEvent -= ExpCounter_ScoringForNoteStarted;
        }
    }
}

