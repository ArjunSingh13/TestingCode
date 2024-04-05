pipeline {
	agent any

	stages {
		stage('Unit tests') {
			steps {
				timeout(time: 7, unit: 'MINUTES') {
					echo 'Building..'
					sh 'ls -lR'
					sh 'make all'
				}
			}
		}
		
	}
}