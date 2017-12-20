%%This code is to take input fromfrom GUI and generate inputs for the training of pictorial structures algorithm

clear all;
numimg = 50;                %The number of images in the database
coldim = (numimg * 25) - 1;
cc = csvread('outt2.csv', 0,0,[0 0 coldim 1]);

temp = cc(:);
counttemp = 1;
ptsAll2(1:25,1:2,1:50) = 0;
display(size(temp));
for k = 1:numimg
    for i = 1:25
        ptsAll2(i, :, k) = cc(i + ((k-1)*25), :);
    end
end
ptsAll(1:14,1:2,1:numimg) = 0;  %Mapping of the GUI dataset to form a mat file which can be used by ramanan codes
for q = 1:numimg
    ptsAll(1, :, q) = ptsAll2(18, :, q);
    ptsAll(2, :, q) = ptsAll2(17, :, q);
    ptsAll(3, :, q) = ptsAll2(16, :, q);
    ptsAll(4, :, q) = ptsAll2(12, :, q);
    ptsAll(5, :, q) = ptsAll2(13, :, q);
    ptsAll(6, :, q) = ptsAll2(14, :, q);
    ptsAll(7, :, q) = ptsAll2(10, :, q);
    ptsAll(8, :, q) = ptsAll2(9, :, q);
    ptsAll(9, :, q) = ptsAll2(8, :, q);
    ptsAll(10, :, q) = ptsAll2(4, :, q);
    ptsAll(11, :, q) = ptsAll2(5, :, q);
    ptsAll(12, :, q) = ptsAll2(6, :, q);
    ptsAll(13, :, q) = ptsAll2(20, :, q);
    ptsAll(14, :, q) = ptsAll2(3, :, q);
end

save('labels.mat', 'ptsAll');

