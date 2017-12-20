
numimg = 50;
coldim = (numimg * 25) - 1;
cc = csvread('outt2.csv', 0,0,[0 0 coldim 1]);

temp = cc(:);
counttemp = 1;
display(size(temp));
for k = 1:numimg
    for i = 1:25
        for j = 1:2
            ptsAll(i, j, k) = temp(counttemp);
            counttemp = counttemp + 1;
        end 
    end
end

save('filename.mat', 'ptsAll');

