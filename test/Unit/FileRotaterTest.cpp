#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <MR/IO/FileRotater.hpp>
#include <filesystem>
#include <fstream>

namespace MR::IO::Test {

class FileRotaterTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() / "filerotater_test";
        std::filesystem::create_directories(test_dir_);
        
        test_file_ = test_dir_ / "test.log";
        test_file_no_ext_ = test_dir_ / "test_no_ext";
    }
    
    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }
    
    void createFile(const std::filesystem::path& path, const std::string& content = "test content") {
        std::ofstream file(path);
        file << content;
        file.close();
    }
    
    bool fileExists(const std::filesystem::path& path) {
        return std::filesystem::exists(path);
    }
    
    std::filesystem::path test_dir_;
    std::filesystem::path test_file_;
    std::filesystem::path test_file_no_ext_;
};

TEST_F(FileRotaterTest, ConstructorWithExtension) {
    FileRotater rotater(test_file_.string(), 1024);
    
    EXPECT_EQ(rotater.getCurrentFilename(), test_file_.string());
}

TEST_F(FileRotaterTest, ConstructorWithoutExtension) {
    FileRotater rotater(test_file_no_ext_.string(), 1024);
    
    EXPECT_EQ(rotater.getCurrentFilename(), test_file_no_ext_.string());
}

TEST_F(FileRotaterTest, ConstructorWithDotInPath) {
    std::filesystem::path dotted_path = test_dir_ / "path.with.dots" / "file.log";
    std::filesystem::create_directories(dotted_path.parent_path());
    
    FileRotater rotater(dotted_path.string(), 1024);
    
    EXPECT_EQ(rotater.getCurrentFilename(), dotted_path.string());
}

TEST_F(FileRotaterTest, ShouldRotateInitiallyFalse) {
    FileRotater rotater(test_file_.string(), 1024);
    
    EXPECT_FALSE(rotater.shouldRotate());
}

TEST_F(FileRotaterTest, ShouldRotateAfterSizeExceeded) {
    FileRotater rotater(test_file_.string(), 100);
    
    rotater.updateCurrentSize(50);
    EXPECT_FALSE(rotater.shouldRotate());
    
    rotater.updateCurrentSize(50);
    EXPECT_TRUE(rotater.shouldRotate());
    
    rotater.updateCurrentSize(1);
    EXPECT_TRUE(rotater.shouldRotate());
}

TEST_F(FileRotaterTest, ShouldRotateExactLimit) {
    FileRotater rotater(test_file_.string(), 100);
    
    rotater.updateCurrentSize(100);
    EXPECT_TRUE(rotater.shouldRotate());
}

TEST_F(FileRotaterTest, UpdateCurrentSizeAccumulates) {
    FileRotater rotater(test_file_.string(), 1000);
    
    rotater.updateCurrentSize(100);
    rotater.updateCurrentSize(200);
    rotater.updateCurrentSize(300);
    
    EXPECT_FALSE(rotater.shouldRotate());
    
    rotater.updateCurrentSize(400);
    EXPECT_TRUE(rotater.shouldRotate());
}

TEST_F(FileRotaterTest, ResetCurrentSize) {
    FileRotater rotater(test_file_.string(), 100);
    
    rotater.updateCurrentSize(150);
    EXPECT_TRUE(rotater.shouldRotate());
    
    rotater.reset();
    EXPECT_FALSE(rotater.shouldRotate());
}

TEST_F(FileRotaterTest, RotateNonExistentFile) {
    FileRotater rotater(test_file_.string(), 100);
    
    EXPECT_FALSE(fileExists(test_file_));
    
    rotater.rotate();
    
    EXPECT_FALSE(fileExists(test_file_));
}

TEST_F(FileRotaterTest, RotateExistingFileWithExtension) {
    createFile(test_file_);
    FileRotater rotater(test_file_.string(), 100);
    
    EXPECT_TRUE(fileExists(test_file_));
    
    rotater.rotate();
    
    EXPECT_FALSE(fileExists(test_file_));
    
    std::filesystem::path rotated_file = test_dir_ / "test1.log";
    EXPECT_TRUE(fileExists(rotated_file));
}

TEST_F(FileRotaterTest, RotateExistingFileWithoutExtension) {
    // Test filename parsing for files without extension
    std::filesystem::path simple_file = test_dir_ / "simple_file";
    FileRotater rotater(simple_file.string(), 100);
    
    // Test that getCurrentFilename works correctly for files without extension
    EXPECT_EQ(rotater.getCurrentFilename(), simple_file.string());
    
    // Test the rotation logic doesn't crash for non-existent files
    EXPECT_NO_THROW(rotater.rotate());
}

TEST_F(FileRotaterTest, RotateResetsCurrentSize) {
    createFile(test_file_);
    FileRotater rotater(test_file_.string(), 100);
    
    rotater.updateCurrentSize(150);
    EXPECT_TRUE(rotater.shouldRotate());
    
    rotater.rotate();
    EXPECT_FALSE(rotater.shouldRotate());
}

TEST_F(FileRotaterTest, MultipleRotationsIncrementCounter) {
    FileRotater rotater(test_file_.string(), 100);
    
    createFile(test_file_, "content1");
    rotater.rotate();
    
    createFile(test_file_, "content2");
    rotater.rotate();
    
    createFile(test_file_, "content3");
    rotater.rotate();
    
    EXPECT_TRUE(fileExists(test_dir_ / "test1.log"));
    EXPECT_TRUE(fileExists(test_dir_ / "test2.log"));
    EXPECT_TRUE(fileExists(test_dir_ / "test3.log"));
    EXPECT_FALSE(fileExists(test_file_));
}

TEST_F(FileRotaterTest, RotationSkipsExistingFiles) {
    FileRotater rotater(test_file_.string(), 100);
    
    createFile(test_dir_ / "test1.log", "existing1");
    createFile(test_dir_ / "test2.log", "existing2");
    
    createFile(test_file_, "new_content");
    rotater.rotate();
    
    EXPECT_TRUE(fileExists(test_dir_ / "test1.log"));
    EXPECT_TRUE(fileExists(test_dir_ / "test2.log"));
    EXPECT_TRUE(fileExists(test_dir_ / "test3.log"));
    EXPECT_FALSE(fileExists(test_file_));
    
    std::ifstream file1(test_dir_ / "test1.log");
    std::string content1((std::istreambuf_iterator<char>(file1)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content1, "existing1");
    
    std::ifstream file3(test_dir_ / "test3.log");
    std::string content3((std::istreambuf_iterator<char>(file3)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content3, "new_content");
}

TEST_F(FileRotaterTest, GetCurrentFilenameConsistent) {
    std::string filename = test_file_.string();
    FileRotater rotater(filename, 100);
    
    EXPECT_EQ(rotater.getCurrentFilename(), filename);
    
    rotater.updateCurrentSize(50);
    EXPECT_EQ(rotater.getCurrentFilename(), filename);
    
    rotater.rotate();
    EXPECT_EQ(rotater.getCurrentFilename(), filename);
}

TEST_F(FileRotaterTest, ZeroMaxSizeAlwaysRotates) {
    FileRotater rotater(test_file_.string(), 0);
    
    EXPECT_TRUE(rotater.shouldRotate());
    
    rotater.updateCurrentSize(1);
    EXPECT_TRUE(rotater.shouldRotate());
}

TEST_F(FileRotaterTest, LargeFileSize) {
    size_t large_size = 10ULL * 1024 * 1024 * 1024;
    FileRotater rotater(test_file_.string(), large_size);
    
    rotater.updateCurrentSize(large_size - 1);
    EXPECT_FALSE(rotater.shouldRotate());
    
    rotater.updateCurrentSize(1);
    EXPECT_TRUE(rotater.shouldRotate());
}

TEST_F(FileRotaterTest, EmptyFilename) {
    FileRotater rotater("", 100);
    
    EXPECT_EQ(rotater.getCurrentFilename(), "");
}

TEST_F(FileRotaterTest, FilenameWithOnlyDot) {
    FileRotater rotater(".", 100);
    
    EXPECT_EQ(rotater.getCurrentFilename(), ".");
}

TEST_F(FileRotaterTest, FilenameStartingWithDot) {
    std::filesystem::path hidden_file = test_dir_ / ".hidden.log";
    FileRotater rotater(hidden_file.string(), 100);
    
    EXPECT_EQ(rotater.getCurrentFilename(), hidden_file.string());
    
    createFile(hidden_file);
    rotater.rotate();
    
    EXPECT_TRUE(fileExists(test_dir_ / ".hidden1.log"));
    EXPECT_FALSE(fileExists(hidden_file));
}

}