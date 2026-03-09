#include <QtTest>
#include <QString>
#include <QStandardPaths>
#include <QDir>

#include "../include/ModelManager.h"

using MCFG::ModelManager;

class ModelManagerTest : public QObject
{
    Q_OBJECT

public:
    ModelManagerTest() = default;
    ~ModelManagerTest() = default;

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testLoadModels();
    void testAvailableModelNames();
    void testModelState_ValidModel();
    void testModelState_MissingOnnx();
    void testModelState_InvalidConfig();
    void testActivateModel_Valid();
    void testActivateModel_Invalid();
    void testActivateModel_AlreadyActive();
    void testSetClassSelected();
    void testModelState_NoModelPathSpecified();
    void testNameFilter_Default();
    void testNameFilter_Wildcard();
    void testNameFilter_NoMatches();

private:
    QString testDataDir;
    std::unique_ptr<ModelManager> manager;
};

void ModelManagerTest::initTestCase()
{
    // Get the test data directory - it's relative to the test executable
    testDataDir = QString(TEST_DATA_DIR);
    QVERIFY(!testDataDir.isEmpty());
    QVERIFY(QDir(testDataDir).exists());
}

void ModelManagerTest::cleanupTestCase()
{
    // Cleanup if needed
}

void ModelManagerTest::testLoadModels()
{
    manager = std::make_unique<ModelManager>(testDataDir);
    
    // Should have loaded some models
    const auto& models = manager->models();
    QVERIFY(models.size() > 0);
    
    // Check that we have the expected models
    bool hasValid1 = false;
    bool hasValid2 = false;
    bool hasMissingOnnx = false;
    bool hasInvalidConfig = false;
    
    for (const auto& entry : models) {
        if (entry.name == "valid_model1") hasValid1 = true;
        if (entry.name == "valid_model2") hasValid2 = true;
        if (entry.name == "missing_onnx") hasMissingOnnx = true;
        if (entry.name == "invalid_config") hasInvalidConfig = true;
    }
    
    QVERIFY(hasValid1);
    QVERIFY(hasValid2);
    QVERIFY(hasMissingOnnx);
    QVERIFY(hasInvalidConfig);
}

void ModelManagerTest::testAvailableModelNames()
{
    if (!manager) {
        manager = std::make_unique<ModelManager>(testDataDir);
    }
    
    auto availableModels = manager->availableModelNames();
    
    // Should only contain valid models
    QVERIFY(availableModels.contains("valid_model1"));
    QVERIFY(availableModels.contains("valid_model2"));
    QVERIFY(availableModels.contains("no_model_path")); // This model has no path but is valid
    
    // Invalid models should not be in available list
    QVERIFY(!availableModels.contains("missing_onnx"));
    QVERIFY(!availableModels.contains("invalid_config"));
    
    // Should be exactly 3 valid models
    QCOMPARE(availableModels.size(), 3);
}

void ModelManagerTest::testModelState_ValidModel()
{
    if (!manager) {
        manager = std::make_unique<ModelManager>(testDataDir);
    }
    
    auto state = manager->modelState("valid_model1");
    QCOMPARE(state, ModelManager::ModelState::Ready);
    
    auto error = manager->modelError("valid_model1");
    QVERIFY(error.isEmpty());
}

void ModelManagerTest::testModelState_MissingOnnx()
{
    if (!manager) {
        manager = std::make_unique<ModelManager>(testDataDir);
    }
    
    auto state = manager->modelState("missing_onnx");
    QCOMPARE(state, ModelManager::ModelState::MissingModel);
    
    auto error = manager->modelError("missing_onnx");
    QVERIFY(!error.isEmpty());
    QVERIFY(error.contains("not found"));
}

void ModelManagerTest::testModelState_InvalidConfig()
{
    if (!manager) {
        manager = std::make_unique<ModelManager>(testDataDir);
    }
    
    auto state = manager->modelState("invalid_config");
    QCOMPARE(state, ModelManager::ModelState::InvalidConfig);
    
    auto error = manager->modelError("invalid_config");
    QVERIFY(!error.isEmpty());
}

void ModelManagerTest::testActivateModel_Valid()
{
    if (!manager) {
        manager = std::make_unique<ModelManager>(testDataDir);
    }
    
    auto result = manager->activateModel("valid_model1");
    QVERIFY(result.has_value());
    
    auto od = result.value();
    QCOMPARE(od.name, QString("valid_model1"));

    QVERIFY(od.config != nullptr);
    QVERIFY(od.state == ModelManager::ModelState::Ready);
    
    // Check that we can access classes
    const auto& classes = od.config->getClasses();
    QCOMPARE(classes.size(), 3); // Should have 3 classes from the config
}

void ModelManagerTest::testActivateModel_Invalid()
{
    if (!manager) {
        manager = std::make_unique<ModelManager>(testDataDir);
    }
    
    // Try to activate invalid model
    auto result = manager->activateModel("invalid_config");
    QVERIFY(!result.has_value());
    QVERIFY(manager->activeModelName().isEmpty());
    QCOMPARE(manager->activeModel(), std::nullopt);
    QCOMPARE(manager->modelState("invalid_config"), ModelManager::ModelState::InvalidConfig);
    
    // Try to activate non-existent model
    auto result2 = manager->activateModel("does_not_exist");
    QVERIFY(!result2.has_value());
    QVERIFY(manager->activeModelName().isEmpty());
    QCOMPARE(manager->activeModel(), std::nullopt);
}

void ModelManagerTest::testActivateModel_AlreadyActive()
{
    if (!manager) {
        manager = std::make_unique<ModelManager>(testDataDir);
    }
    
    // Activate a model
    auto result1 = manager->activateModel("valid_model1");
    QVERIFY(result1.has_value());
    auto entry1 = result1.value();
    
    // Activate the same model again - should return the same instance
    auto result2 = manager->activateModel("valid_model1");
    QVERIFY(result2.has_value());
    auto entry2 = result2.value();
    
    QCOMPARE(entry1.name, entry2.name);
    QCOMPARE(entry1.config.get(), entry2.config.get());
    
    // Activate a different model
    auto result3 = manager->activateModel("valid_model2");
    QVERIFY(result3.has_value());
    auto entry3 = result3.value();
    
    QVERIFY(entry3.config.get() != entry1.config.get());
    QCOMPARE(entry3.name, QString("valid_model2"));
}

void ModelManagerTest::testSetClassSelected()
{
    if (!manager) {
        manager = std::make_unique<ModelManager>(testDataDir);
    }
    
    // Activate valid_model1 which has classes with ids 1, 2, 3
    auto result = manager->activateModel("valid_model1");
    QVERIFY(result.has_value());
    auto entry = result.value();
    
    // Test setting class as selected
    bool updated = manager->setClassSelected("valid_model1", 1, true);
    QVERIFY(updated);
    
    // Verify the class is selected
    auto class1 = entry.config->getClassById(1);
    QVERIFY(class1 != nullptr);
    QVERIFY(class1->selected == true);
    
    // Test unselecting the class
    updated = manager->setClassSelected("valid_model1", 1, false);
    QVERIFY(updated);
    
    auto class1_unselected = entry.config->getClassById(1);
    QVERIFY(class1_unselected != nullptr);
    QVERIFY(class1_unselected->selected == false);
    
    // Test setting non-existent class
    updated = manager->setClassSelected("valid_model1", 999, true);
    QVERIFY(!updated);
}

void ModelManagerTest::testModelState_NoModelPathSpecified()
{
    if (!manager) {
        manager = std::make_unique<ModelManager>(testDataDir);
    }
    
    // Model with no model_path specified in JSON should use default .onnx file
    auto state = manager->modelState("no_model_path");
    QCOMPARE(state, ModelManager::ModelState::Ready);
    
    auto error = manager->modelError("no_model_path");
    QVERIFY(error.isEmpty());
    
    // Should be able to activate it
    auto result = manager->activateModel("no_model_path");
    QVERIFY(result.has_value());
}

void ModelManagerTest::testNameFilter_Default()
{
    // Create manager without filter - should load all models
    auto mgr = std::make_unique<ModelManager>(testDataDir);
    
    // Check that default filter is empty
    QCOMPARE(mgr->nameFilter(), QString());
    
    // Should have loaded all models (5 total: valid_model1, valid_model2, 
    // missing_onnx, invalid_config, no_model_path)
    const auto& models = mgr->models();
    QCOMPARE(models.size(), 5);
}

void ModelManagerTest::testNameFilter_Wildcard()
{
    // Create manager with filter pattern for valid_* models
    auto mgr = std::make_unique<ModelManager>(testDataDir);
    mgr->setNameFilter("valid_*");
    
    // Check that filter is set correctly
    QCOMPARE(mgr->nameFilter(), QString("valid_*"));
    
    // Should only have valid_model1 and valid_model2
    const auto& models = mgr->models();
    QCOMPARE(models.size(), 2);
    
    // Verify the names match the pattern
    auto names = mgr->availableModelNames();
    QVERIFY(names.contains("valid_model1"));
    QVERIFY(names.contains("valid_model2"));
    QVERIFY(!names.contains("missing_onnx"));
    QVERIFY(!names.contains("invalid_config"));
    QVERIFY(!names.contains("no_model_path"));
}

void ModelManagerTest::testNameFilter_NoMatches()
{
    // Create manager with filter that matches nothing
    auto mgr = std::make_unique<ModelManager>(testDataDir);
    mgr->setNameFilter("nonexistent_*");
    
    // Check that filter is set correctly
    QCOMPARE(mgr->nameFilter(), QString("nonexistent_*"));
    
    // Should have no models loaded
    const auto& models = mgr->models();
    QCOMPARE(models.size(), 0);
    
    auto names = mgr->availableModelNames();
    QVERIFY(names.isEmpty());
    
    // Trying to activate any model should fail
    auto model = mgr->activateModel("valid_model1");
    QVERIFY(!model.has_value());
    QVERIFY(mgr->modelError("valid_model1").contains("Unknown model"));
}

QTEST_MAIN(ModelManagerTest)
#include "ModelManagerTest.moc"
