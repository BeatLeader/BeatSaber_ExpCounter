using ExpCounter.Configuration;
using IPA;
using IPA.Config;
using IPA.Config.Stores;
using SiraUtil.Zenject;
using IPALogger = IPA.Logging.Logger;

namespace ExpCounter
{
    [Plugin(RuntimeOptions.DynamicInit), NoEnableDisable]
    public class Plugin
    {
        internal static Plugin Instance { get; private set; }

        [Init]
        public Plugin(IPALogger logger, Config conf, Zenjector zenject)
        {
            zenject.UseLogger(logger);
            zenject.UseMetadataBinder<Plugin>();
            PluginConfig.Instance = conf.Generated<PluginConfig>();
        }
    }
}
