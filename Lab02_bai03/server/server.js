const express = require('express');
const multer = require('multer');
const path = require('path');

const app = express();
//const route = app.router
const port = 8000;

// Thư mục lưu trữ các bản cập nhật
const uploadDirectory = './updates';

// Cấu hình Multer để xử lý tệp tải lên
const storage = multer.diskStorage({
    destination: (req, file, cb) => {
        cb(null, uploadDirectory);
    },
    filename: (req, file, cb) => {
        cb(null, file.originalname);
    },
});

const update = multer({ storage: storage });

// Đảm bảo thư mục lưu trữ tồn tại
const fs = require('fs');
if (!fs.existsSync(uploadDirectory)) {
    fs.mkdirSync(uploadDirectory);
}

// Trang chủ hiển thị danh sách các bản cập nhật
app.get('/', (req, res) => {
    const updateFiles = fs.readdirSync(uploadDirectory);
    res.sendFile(path.join(__dirname, 'index.html'));
});

  // Route để lấy danh sách các tệp đã upload
app.get('/getUpdateFiles', (req, res) => {
    const updateFiles = fs.readdirSync(uploadDirectory);
    res.json({ files: updateFiles });
  });

app.post('/deleteFile', (req, res) => {
    // Lấy danh sách tên các tệp trong thư mục
    const files = fs.readdirSync(uploadDirectory);

    // Duyệt qua danh sách tên tệp và xóa từng tệp
    files.forEach((file) => {
        const filePath = `${uploadDirectory}/${file}`;
        try {
            fs.unlinkSync(filePath)
            console.log(`Đã xóa tệp: ${filePath}`);
        } catch (err) {
            console.error(`Lỗi khi xóa tệp: ${filePath}`);
        }
    });
  

    //console.log('Đã xóa tất cả các tệp trong thư mục.');
    res.json({ success: true });
});
// Route xử lý tải lên tệp
app.post('/update', update.single('update'), (req, res) => {
   res.redirect('/');
});

// Thêm body-parser để xử lý JSON
const bodyParser = require('body-parser');
app.use(bodyParser.json());

app.post('/updateConfig', (req, res) => {
    const { name, path, version } = req.body;
    const updatedPath = 'updates/' + path;
    // Create the updated configuration object
    const updatedConfig = {
        name,
        path: updatedPath,
        version
    };

    // Use the asynchronous fs.writeFile method to write the updated data to config.json
    fs.writeFile('./config.json', JSON.stringify(updatedConfig, null, 2), (err) => {
        if (err) {
            // Handle any errors that occurred during file writing
            console.error(err);
            res.status(500).json({ message: 'Failed to update config.' });
        } else {
            // File was written successfully
            res.json({ message: 'Config has been updated successfully.' });
        }
    });
});


app.listen(port, () => {
    console.log(`Server đang chạy tại http://localhost:${port}`);
});

app.use(express.static(__dirname));
