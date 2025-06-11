#include <QtTest>
#include <Tensor.h>

using namespace NN;

class TensorConversionTest : public QObject
{
    Q_OBJECT

public:
    TensorConversionTest() = default;
    ~TensorConversionTest() = default;

private slots:
    void testData2D();
    void testData3D();
    void testCvMat2D();
    void testCvMat3D();
    void testMatToMat();
};

void TensorConversionTest::testData2D()
{
    // Create a 2D tensor with shape (2, 3)
    std::vector<float> data2d = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    Shape shape2d = {2, 3}; // 2 rows, 3 columns
    auto tensor2d = Tensor::fromData(std::move(data2d), shape2d);
    QVERIFY(tensor2d.has_value());
    QCOMPARE(tensor2d->shape(), shape2d);
    QCOMPARE(tensor2d->numElements(), 6);
    QCOMPARE(tensor2d->toVector<float>().value(), std::vector<float>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}));
}

void TensorConversionTest::testData3D()
{
    // Create a 3D tensor with shape (2, 3, 2)
    std::vector<float> data3d = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f,
                                 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f};
    Shape shape3d = {2, 3, 2}; // 2 channels, 3 height, 2 width
    auto tensor3d = Tensor::fromData(std::move(data3d), shape3d);
    QVERIFY(tensor3d.has_value());
    QCOMPARE(tensor3d->shape(), shape3d);
    QCOMPARE(tensor3d->numElements(), 12);
    QCOMPARE(tensor3d->toVector<float>().value(), std::vector<float>({1.0f, 2.0f, 3.0f, 4.0f,
                                                                      5.0f, 6.0f, 7.0f, 8.0f,
                                                                      9.0f, 10.0f, 11.0f, 12.0f}));
}

void TensorConversionTest::testCvMat2D()
{
    // Create a cv::Mat and convert it to a 2D tensor
    cv::Mat matLeft = cv::Mat(cv::Size(3, 2), CV_32F, cv::Scalar(1.0f));
    cv::Mat matRight = cv::Mat(cv::Size(3, 2), CV_32F, cv::Scalar(2.0f));
    cv::Mat mat2d;
    cv::hconcat(matLeft, matRight, mat2d);
    QCOMPARE(mat2d.rows, 2);
    QCOMPARE(mat2d.cols, 6);
    QCOMPARE(mat2d.type(), CV_32F);

    auto tensor2d = Tensor::fromCvMat(mat2d);
    QVERIFY(tensor2d.has_value());
    QCOMPARE(tensor2d->shape(), Shape({1, 2, 6})); // [N]CHW format
    QCOMPARE(tensor2d->toVector<float>().value(), std::vector<float>({1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 2.0f,
                                                                      1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 2.0f}));
}

void TensorConversionTest::testCvMat3D()
{
    // Create a cv::Mat and convert it to a 3D tensor
    cv::Mat matR = cv::Mat(cv::Size(2,3),CV_8U, cv::Scalar(120));
    cv::Mat matG = cv::Mat(cv::Size(2,3),CV_8U, cv::Scalar(30));
    cv::Mat matB = cv::Mat(cv::Size(2,3),CV_8U, cv::Scalar(200));
    cv::Mat mat3d;
    cv::merge(std::vector<cv::Mat>{matR, matG, matB}, mat3d);

    QCOMPARE(mat3d.rows, 3);
    QCOMPARE(mat3d.cols, 2);
    QCOMPARE(mat3d.type(), CV_8UC3);

    auto tensor3d = Tensor::fromCvMat(mat3d);
    QVERIFY(tensor3d.has_value());
    QCOMPARE(tensor3d->shape(), Shape({3, 3, 2})); // [N]CHW format
    QCOMPARE(tensor3d->numElements(), 18);
    QVERIFY(tensor3d->dtype() == TensorType::UInt8);

    QCOMPARE(tensor3d->toVector<uint8_t>().value(), std::vector<uint8_t>({120, 120, 120, 120, 120, 120,
                                                                          30, 30, 30, 30, 30, 30,
                                                                          200, 200, 200, 200, 200, 200}));
}

void TensorConversionTest::testMatToMat()
{
    // Create a cv::Mat and convert it to a 3D tensor
    cv::Mat matR = cv::Mat(cv::Size(2,3),CV_8U, cv::Scalar(120));
    cv::Mat matG = cv::Mat(cv::Size(2,3),CV_8U, cv::Scalar(30));
    cv::Mat matB = cv::Mat(cv::Size(2,3),CV_8U, cv::Scalar(200));
    cv::Mat mat3d;
    cv::merge(std::vector<cv::Mat>{matR, matG, matB}, mat3d);

    QCOMPARE(mat3d.rows, 3);
    QCOMPARE(mat3d.cols, 2);
    QCOMPARE(mat3d.type(), CV_8UC3);

    auto tensor3d = Tensor::fromCvMat(mat3d);
    QVERIFY(tensor3d.has_value());
    QCOMPARE(tensor3d->shape(), Shape({3, 3, 2})); // [N]CHW format
    QCOMPARE(tensor3d->numElements(), 18);
    QVERIFY(tensor3d->dtype() == TensorType::UInt8);

    auto matFromTensor = tensor3d->toCvMat();
    QVERIFY(matFromTensor.has_value());
    QCOMPARE(matFromTensor->rows, 3);
    QCOMPARE(matFromTensor->cols, 2);
    QCOMPARE(matFromTensor->type(), CV_8UC3);

    // Compare the original and converted matrices
    cv::Mat diff = mat3d != *matFromTensor;
    cv::Mat flattened = diff.reshape(1, 1); // Flatten the diff matrix to a single row
    int nonZeroCount = cv::countNonZero(flattened);
    QVERIFY(nonZeroCount == 0); // Ensure no differences
}

QTEST_MAIN(TensorConversionTest)
#include "TensorConversion.moc"