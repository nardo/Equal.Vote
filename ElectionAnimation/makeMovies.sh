ffmpeg -framerate 15 -i yee_%d_0.png -pix_fmt yuv420p yee_0.mp4
ffmpeg -framerate 15 -i yee_%d_1.png -pix_fmt yuv420p yee_1.mp4
ffmpeg -framerate 15 -i yee_%d_2.png -pix_fmt yuv420p yee_2.mp4
ffmpeg -framerate 15 -i yee_%d_3.png -pix_fmt yuv420p yee_3.mp4
ffmpeg -framerate 15 -i yee_%d_4.png -pix_fmt yuv420p yee_4.mp4
ffmpeg -framerate 15 -i yee_%d_5.png -pix_fmt yuv420p yee_5.mp4

ffmpeg -framerate 15 -i yee_diff_%d_0.png -pix_fmt yuv420p yee_diff_0.mp4
ffmpeg -framerate 15 -i yee_diff_%d_1.png -pix_fmt yuv420p yee_diff_1.mp4
ffmpeg -framerate 15 -i yee_diff_%d_2.png -pix_fmt yuv420p yee_diff_2.mp4
ffmpeg -framerate 15 -i yee_diff_%d_3.png -pix_fmt yuv420p yee_diff_3.mp4
ffmpeg -framerate 15 -i yee_diff_%d_5.png -pix_fmt yuv420p yee_diff_5.mp4
