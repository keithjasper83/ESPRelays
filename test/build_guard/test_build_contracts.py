"""Keep retired dependencies out of every declared PlatformIO environment."""
import configparser
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class BuildContractTest(unittest.TestCase):
    def test_every_environment_excludes_retired_dependencies(self):
        config = configparser.ConfigParser(interpolation=None)
        config.read(ROOT / 'platformio.ini')
        environments = [name for name in config if name.startswith('env:')]
        self.assertTrue(environments)
        for name in environments:
            with self.subTest(environment=name):
                # Include shared env settings, so moving a dependency into inheritance cannot bypass this check.
                settings = dict(config['env']) if 'env' in config else {}
                settings.update(config[name])
                self.assertNotRegex(str(settings), r'(?i)PubSubClient|NeoPixel|MqttManager|IndicatorLeds|components/mdns')

    def test_retired_sources_cannot_be_picked_up_by_automatic_discovery(self):
        for name in ('src/MqttManager.cpp', 'src/IndicatorLeds.cpp', 'src/telemetry.cpp',
                     'include/telemetry.h', 'src/telemetry', 'components/mdns'):
            with self.subTest(path=name):
                self.assertFalse((ROOT / name).exists())

    def test_retired_routes_are_not_registered(self):
        source = (ROOT / 'src/WebControlServer.cpp').read_text()
        routes = re.findall(r'gServer\.on\("([^"]+)"', source)
        self.assertIn('/unified/manifest', routes)
        self.assertIn('/on', routes)
        self.assertIn('/temperature/capture-low', routes)
        self.assertFalse(any(route.startswith(('/led/', '/test/')) for route in routes))


if __name__ == '__main__':
    unittest.main()
